import socket
import time
import traceback
import sys
import threading
import random
import logging
import time
import datetime
import signal
from thread import start_new_thread
import argparse
import Queue
import os
import sys

experiment_duration = 600  # seconds
max_silence_interval = 180  # seconds
min_hop_distance = 3

i_max = j_max = 0
i_min = sys.maxint

riots = {} # key: (i,j) coordinate on the Grid. value: (port, (IP, link-local addr))
riots_lock = threading.Lock()
riots_complete = threading.Lock()
riots_ready = threading.Lock()
potential_targnodes = {} # key: (i,j) coordinate on the Grid. value: [(i,j)] nodes that are far enough away
num_ready_riots = num_riots = 0
sockets = []
sockets_lock = threading.Lock()
ports_local_path = "../../riot/desvirt_mehlis/ports.list" # TODO properly
max_shutdown_interval = shutdown_riots = shutdown_window = 0
shutdown_queue = Queue.Queue()
msg_queues = {}

plain_mode = False

def get_ports():
    with open(ports_local_path, 'r') as f:
        ports_local = f.readlines()
        global i_max, j_max, i_min

        for port_string in ports_local:
            lst = port_string.split(",")

            # handle grid
            if (len(lst) is 3):
                i = int(lst[0])
                j = int(lst[1])
                port = lst[2].rstrip('\n')
                riots[(i,j)] = (port, "")
                if (i > i_max):
                    i_max = i
                if (j > j_max):
                    j_max = j

                # handle line
            elif (len(lst) is 2):
                i = abs(int(lst[0]))
                port = lst[1].rstrip('\n')
                riots[(i)] = (port, "")
                if (i > i_max): #positions are negative, warum auch immer
                    i_max = i

                if (i < i_min ): #positions are negative, warum auch immer
                    i_min = i

'''
parse something like
ifconfig
Iface   0   HWaddr: 0x0001 Channel: 0 PAN ID: 0xabcd
            EUI-64: 00-00-00-ff-fe-00-00-01
            Source address mode: short
            Transceivers:
             * native
            inet6 addr: error in conversion
            inet6 addr: ff02::2/128  scope: local [multicast]
            inet6 addr: fe80::ff:fe00:1/128  scope: local
            inet6 addr: ff02::1:0:ff00:1/128  scope: local [multicast]
            inet6 addr: ff02::1/128  scope: local [multicast]
            inet6 addr: ::1/128  scope: local
'''
def get_node_addrs(data):
    lines = data.split("\n");
    ip = ""
    ll_addr = ""

    for line in lines:
        if ("fe80" in line):
            ip = line.strip().split("/")[0].split(" ")[-1]
        elif ("EUI-64" in line):
            ll_addr = line.split(":")[-1].strip()

    return (ip, ll_addr)

def get_shell_output(sock):
    # read IP data until ">" marks termination of the shell output
    data = chunk = ""

    while (not(">" in data)):
        chunk = sock.recv(4096)
        if (chunk == ""):
            return data
        data += chunk
    return data

'''
for each node, determine all nodes in the grid that are >= min_hop_distance away
TODO: oh, ups... inkludiert das nicht auch querverbindungen?
'''
def collect_potential_targnodes():
    global potential_targnodes, min_hop_distance, i_max, i_min
    print "i_max:", i_max, "j_max:", j_max, "min_hop_distance:", min_hop_distance

    for position, connection in riots.iteritems():
        # get positions on grid
        if (type(position) is tuple):
            i = position[0]
            j = position[1]

            i = int(i)
            j = int(j)

            m_lst = range(1, int(i_max)+1)
            n_lst = range(1, int(j_max)+1)

            potential_targnodes[position] = [(m, n) for m in m_lst for n in n_lst if 
                                            ((m >= i+min_hop_distance)or(m <= i-min_hop_distance)) 
                                            or ((n>=j+min_hop_distance)or(n<=j-min_hop_distance))]

        elif (type(position) is int):
                i = position
                m_lst = range(int(i_min), int(i_max)+1)
                potential_targnodes[position] = [m for m in m_lst if 
                                            ((m >= i+min_hop_distance)or(m <= i-min_hop_distance))]
        else:
            print "can't interpret position"

    print "potential_targnodes: ", potential_targnodes

def collect_neighbor_coordinates(position):
    neighbor_coordinates = []

    if (type(position) is tuple):
        i = int(position[0])
        j = int(position[1])

        m_lst = range(1, int(i_max)+1)
        n_lst = range(1, int(j_max)+1)
        
        neighbor_coordinates = [(m,n) for m in m_lst for n in n_lst if ((m == i and ((j-1 == n) or (n == j+1))) or ((m == i+1) or (m == i-1)) and (j-1 <= n <= j+1))]

    elif (type(position) is int):
        i = position
        m_lst = range(i_min, i_max+1)

        neighbor_coordinates = [m for m in m_lst if (m == i+1) or (m == i-1)]
    else:
        print "can't interpret position"

    print "neighbors of ", position, ": ", neighbor_coordinates
    return neighbor_coordinates

def connect_riots():
    global potential_targnodes, riots

    riots_complete.acquire()
    riots_ready.acquire()
    collect_potential_targnodes()

    for position, connection in riots.iteritems():
        start_new_thread(test_sender_thread,(position,connection[0]))
        # make sure the main thread isn't killed before we initialize our sockets
        time.sleep(2) 

    logging.debug("riots: %s\n", riots)
    if (shutdown_riots > 0):
        start_new_thread(test_shutdown_thread,())

    if (plain_mode):
        orignode = random.choice(riots.keys())
        targnode = random.choice(potential_targnodes[orignode])
        targnode_ip = riots[targnode][1][0]
        print "orignode:", orignode, "targnode:", targnode, "targnode_ip", targnode_ip

        msg_queues[orignode].put("send_data %s\n" % targnode_ip)

    # after experiment_duration, this function will exit and kill all the threads it generated.
    time.sleep(experiment_duration) 

def test_sender_thread(position, port):
    sys.stdout.write("Port: %s\n" % port)

    data = my_ip = random_neighbor = ""
    thread_id = threading.currentThread().name
    global shutdown_queue, potential_targnodes, num_ready_riots

    sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)

    with sockets_lock:
        sockets.append(sock)

    try:
        sock.connect(("127.0.0.1 ", int(port)))
        sock.sendall("ifconfig\n")

        # add own message queue to global msg queue dict
        msg_queues[position] = Queue.Queue()

        #get all IP addresses of this RIOT
        data = get_shell_output(sock)

        # check for errors (superduper professionally)
        if ("shell: command not found" in data):
            sys.stdout.write("Hickup while retrieving IPs. Exiting. Please try again.\n")
            sys.exit()

        # get all of my potential TargNodes
        my_targnodes = potential_targnodes[position]

        # get relevant IP address
        my_addrs = get_node_addrs(data)
        my_ip = my_addrs[0]

        logging.debug("{%s} IP: %s, LL-Addr: %s\n" % (thread_id, my_ip, my_addrs[1]))

        with riots_lock:
            sys.stdout.write("adding node with IP %s to riots...\n" % my_ip)
            riots[position] = (port, my_addrs)
            global num_riots
            num_riots += 1

        # wait until all nodes are initialized. if they are, release the lock 
        # so all nodes can start sending
        if (num_riots < len(riots)):
            riots_complete.acquire() # blocking wait

        riots_complete.release()         # open the gate for the next thread

        sys.stdout.write("riots_complete unlocked at %s\n" % thread_id)

        # first, learn about all your neighbors
        my_neighbor_coordinates = collect_neighbor_coordinates(position)
        sys.stdout.write("%s my neighbors: %s\n" % (position, my_neighbor_coordinates))
        
        for neighbor in my_neighbor_coordinates:
            (ip, ll_addr) = riots[neighbor][1]
            #sys.stdout.write("{%s} Adding neighbor %s %s\n" % (thread_id, ip, ll_addr))
            sock.sendall("add_neighbor %s %s\n" % (ip, ll_addr))
            # make sure that went okay and empty shell output buffer
            logging.debug("{%s: %s}\n%s" % (thread_id, my_ip, get_shell_output(sock)))

        num_ready_riots += 1

        if (num_ready_riots < num_riots):
            riots_ready.acquire()
        riots_ready.release() # enable next node to add their neighbors in peace
        #print "type neighbor", type(my_neighbor_coordinates[0]), "type position", type(position)
        sys.stdout.write("riots_ready unlocked at %s\n" % thread_id)

        while (True):
            if (not msg_queues[position].empty()):
                instruction = msg_queues[position].get()
                sys.stdout.write("{%s} received instruction: %s\n" % (thread_id, instruction))
                if ("exit" in instruction):
                    # print last words
                    sock.sendall("sdfghjkl\n")
                    logging.debug("{%s: %s}\n%s" % (thread_id, my_ip, get_shell_output(sock)))

                sock.sendall(instruction)
                logging.debug("{%s: %s}\n%s" % (thread_id, my_ip, get_shell_output(sock)))

            if (not plain_mode):

                #wait for a little while
                some_time = random.randint(1, max_silence_interval)
                time.sleep(some_time)

                try:
                    shutdown_queue.get(False) # we've been told to shut down
                    logging.debug("{%s: %s}\n%s" % (thread_id, my_ip, get_shell_output(sock)))
                    sys.stdout.write("{%s} shutting down\n" % thread_id)

                    # shut down my RIOT
                    sock.sendall("exit\n")

                    sys.stdout.write("number of my neighbors: %s\n" % len(my_neighbor_coordinates))
                    # emulate NDP of my 1-hop-neighbors noticing my shutdown
                    for neighbor in my_neighbor_coordinates:
                        msg_queues[neighbor].put("rm_neighbor %s\n" % my_ip)
                        sys.stdout.write("{%s} notifying neighbor %s of my death\n" % (thread_id, neighbor))

                    sys.exit()
                    self.process.terminate() # TODO does this do the trick?

                except:
                    # no shutdown instruction, continue as usual
                    # pick random node
                    with riots_lock:
                        # only attempt to send if potential targnodes exist
                        if (len(my_targnodes) > 0):
                            targnode = random.choice(my_targnodes)
                            targnode_ip = riots[targnode][1][0]

                            logging.debug("{%s: %s, %s} send_data to %s %s\n" % (thread_id, my_ip, position, targnode_ip, targnode))
                            sock.sendall("send_data %s\n" % targnode_ip)
                            logging.debug("{%s: %s, %s}\n%s" % (thread_id, my_ip, position, get_shell_output(sock)))

    except:
        traceback.print_exc()
        sys.exit()

def test_shutdown_thread():
    global shutdown_riots
    global shutdown_queue

    #wait until everybody is ready
    riots_complete.acquire() 
    riots_complete.release()

    # actually start sending
    riots_ready.acquire()
    riots_ready.release()

    time.sleep(shutdown_window * 2)

    while (shutdown_riots > 0):
        # TODO use msg_queues also
        sys.stdout.write("shutting down random node\n")

        shutdown_queue.put("foo") #doesn't matter what's in there, as long as it's *something*
        shutdown_riots -= 1

        # wait a little while
        some_time = random.randint(1, max_shutdown_interval)
        time.sleep(some_time)

# kill tcp connections on SIGINT
def signal_handler(signal, frame):
    close_connections()
    sys.exit(0)

def close_connections():
    global shutdown_riots

    print "\nCleaning up..."

    for position in riots.keys():
        print "shutting down",  position
        msg_queues[position].put("exit\n")

    # wait until everything has been logged
    time.sleep(10)

    print "Closing sockets..."
    with sockets_lock:
        for socket in sockets:
                socket.close
                print "socket",socket.fileno(),"closed."

    print "done"

def main():
    global shutdown_riots, max_silence_interval, experiment_duration, max_shutdown_interval, min_hop_distance, shutdown_window, plain_mode

    timestamp = time.time()
    signal.signal(signal.SIGINT, signal_handler)

    parser = argparse.ArgumentParser(description='Initiate traffic on a vnet of RIOTs.')
    parser.add_argument('-d','--debug', action='store_true', help='print debug output to console rather than to a logfile')
    parser.add_argument('-s','--shutdown', type = int, help='randomly shut down n nodes during execution')
    parser.add_argument('-t','--time', type = int, help='duration of the experiment (in seconds)')
    parser.add_argument('-i','--interval', type = int, help='max time interval between packet transmissions (in seconds)')
    parser.add_argument('-m','--min_hop_dist', type = int, help='minimum distance between originating and target node (in hops)')    
    parser.add_argument('-p','--plain', action='store_true', help='Only start one transmission')    

    args = parser.parse_args()
    
    if (args.debug):
        print "ALL OUTPUT GENERATED WILL NOT BE STORED IN A LOGFILE."
        logging.basicConfig(level=logging.DEBUG, format='%(asctime)s %(message)s', datefmt='%d-%m-%Y %H:%M:%S')
    else:
        if (not os.path.exists("./logs")):
            os.makedirs("./logs")
        
        date = datetime.datetime.fromtimestamp(timestamp).strftime('%d-%m-%Y %H:%M:%S')
        logfile_name = "logs/auto_test "+date+".log"
        
        print "writing logs to", logfile_name
        logging.basicConfig(filename=logfile_name, level=logging.DEBUG, format='%(asctime)s %(message)s', datefmt='%d-%m-%Y %H:%M:%S')

    if (args.time):
        experiment_duration = args.time
    else:
        experiment_duration = 200

    if (args.interval):
        max_silence_interval = args.interval
    else:
        max_silence_interval = 20

    if (args.min_hop_dist):
        min_hop_distance = args.min_hop_dist
        print "min_hop_dist:", min_hop_distance

    if (args.shutdown > 0):
        shutdown_riots = args.shutdown
        #only shut down RIOTs in the last 3rd of the experiment 
        # so that there actually are routes to repair
        shutdown_window = experiment_duration / 3
        max_shutdown_interval = shutdown_window / shutdown_riots
        print "shutdowns start after", shutdown_window * 2, "seconds" 

    if (args.plain):
        plain_mode = True


    sys.stdout.write("Starting %i seconds of testing...\n" % experiment_duration)

    get_ports()
    connect_riots()

    close_connections()

if __name__ == "__main__":
    main()