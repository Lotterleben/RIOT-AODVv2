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

experiment_duration = 600  # seconds
max_silence_interval = 180  # seconds
min_hop_distance = 3

i_max = j_max = 0

riots = {} # key: (i,j) coordinate on the Grid. value: (port, IP)
riots_lock = threading.Lock()
riots_complete = threading.Lock()
potential_targnodes = {} # key: (i,j) coordinate on the Grid. value: [(i,j)] nodes that are far enough away
num_riots = 0
sockets = []
sockets_lock = threading.Lock()
ports_local_path = "../../../riot/desvirt_mehlis/ports.list" # TODO properly
max_shutdown_interval = shutdown_riots = shutdown_window = 0
shutdown_queue = Queue.Queue()

def get_ports():
    with open(ports_local_path, 'r') as f:
        ports_local = f.readlines()
        global i_max, j_max

        for port_string in ports_local:
            lst = port_string.split(",")
            i = int(lst[0])
            j = int(lst[1])
            port = lst[2].rstrip('\n')
            riots[(i,j)] = (port, "")
            if (i > i_max):
                i_max = i
            if (j > j_max):
                j_max = j

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
def get_node_ip(data):
    lines = data.split("inet6 addr:");
    #relevant_lines = [l for l in lines if ("fe80" in l)]
    for line in lines:
        if ("fe80" in line):
            return line.strip().split("/")[0]

def get_shell_output(sock):
    # read IP data until ">" marks termination of the shell output
    data = chunk = ""

    while (not(">" in data)):
        chunk = sock.recv(4096)
        data += chunk
    return data

'''
for each node, determine all nodes in the grid that are >= min_hop_distance away
TODO: oh, ups... inkludiert das nichtauch querverbindungen?
'''
def collect_potential_targnodes():
    global potential_targnodes, min_hop_distance
    print "i_max:", i_max, "j_max:", j_max, "min_hop_distance:", min_hop_distance

    for position, connection in riots.iteritems():
        i = position[0]
        j = position[1]

        i = int(i)
        j = int(j)

        m_lst = range(1, int(i_max)+1)
        n_lst = range(1, int(j_max)+1)

        potential_targnodes[position] = [(m, n) for m in m_lst for n in n_lst if 
                                        ((m >= i+min_hop_distance)or(m <= i-min_hop_distance)) 
                                        or ((n>=j+min_hop_distance)or(n<=j-min_hop_distance))]

    print potential_targnodes

def connect_riots():
    riots_complete.acquire()
    collect_potential_targnodes()

    for position, connection in riots.iteritems():
        start_new_thread(test_sender_thread,(position,connection[0]))
        # make sure the main thread isn't killed before we initialize our sockets
        time.sleep(2) 

    print "riots:", riots
    if (shutdown_riots > 0):
        start_new_thread(test_shutdown_thread,())

    # after experiment_duration, this function will exit and kill all the threads it generated.
    time.sleep(experiment_duration) 

def test_sender_thread(position, port):
    sys.stdout.write("Port: %s\n" % port)

    data = my_ip = random_neighbor = ""
    thread_id = threading.currentThread().name
    global shutdown_queue, potential_targnodes

    sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    with sockets_lock:
        sockets.append(sock)

    try:
        sock.connect(("127.0.0.1 ", int(port)))
        sock.sendall("ifconfig\n")

        #get all IP addresses of this RIOT
        data = get_shell_output(sock)

        # check for errors (superduper professionally)
        if ("shell: command not found" in data):
            sys.stdout.write("Hickup while retrieving IPs. Exiting. Please try again.\n")
            sys.exit()

        # get all of my potential TargNodes
        my_targnodes = potential_targnodes[position]

        # get relevant IP address
        my_ip = get_node_ip(data)
        #sys.stdout.write("{%s} IP: %s\n" % (thread_id, my_ip))
        logging.debug("{%s} IP: %s\n" % (thread_id, my_ip))

        with riots_lock:
            sys.stdout.write("adding node with IP %s to riots...\n" % my_ip)
            riots[position] = (port, my_ip)
            global num_riots
            num_riots += 1
            sys.stdout.write("num_riots %i\n" % num_riots)

        # wait until all nodes are initialized. if they are, release the lock 
        # so all nodes can start sending
        if (num_riots < len(riots)):
            riots_complete.acquire() # blocking wait

        riots_complete.release()         # open the gate for the next thread

        sys.stdout.write("unlocked %s\n" % thread_id)

        while (True):
            #wait for a little while
            some_time = random.randint(1, max_silence_interval)
            time.sleep(some_time)

            try:
                shutdown_queue.get(False) # we've been told to shut down
                sys.stdout.write("%s shutting down\n" % thread_id)
                sock.sendall("exit\n")
                sys.exit()

            except:
                # no shutdown instruction, continue as usual
                # pick random node
                with riots_lock:
                    targnode = random.choice(my_targnodes)
                    targnode_ip = riots[targnode][1]

                    sys.stdout.write("new random neighbor: %s\n" % targnode_ip)

                    sys.stdout.write("%s Say hi to   %s\n\n" % (port, targnode_ip))
                    sock.sendall("send_data %s\n" % targnode_ip)

                    logging.debug("{%s} %s\n%s" % (thread_id, my_ip, get_shell_output(sock))) # output might not be complete, though...

    except:
        traceback.print_exc()
        sys.exit()

def test_shutdown_thread():
    global shutdown_riots
    global shutdown_queue

    riots_complete.acquire() #wait until everybody is ready
    riots_complete.release()

    time.sleep(shutdown_window * 2) 

    while (shutdown_riots > 0):
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
        print "\nCleaning up..."
            
        with sockets_lock:
            for socket in sockets:
                    socket.close
                    print "socket",socket.fileno(),"closed."

        print "done"

def main():
    global shutdown_riots, max_silence_interval, experiment_duration, max_shutdown_interval, min_hop_distance

    timestamp = time.time()
    signal.signal(signal.SIGINT, signal_handler)

    parser = argparse.ArgumentParser(description='Initiate traffic on a vnet of RIOTs.')
    parser.add_argument('-d','--debug', action='store_true', help='print debug output to console rather than to a logfile')
    parser.add_argument('-s','--shutdown', type = int, help='randomly shut down n nodes during execution')
    parser.add_argument('-t','--time', type = int, help='duration of the experiment (in seconds)')
    parser.add_argument('-i','--interval', type = int, help='max time interval between packet transmissions (in seconds)')
    parser.add_argument('-m','--min_hop_dist', type = int, help='minimum distance between originating and target node (in hops)')    

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
        print shutdown_riots


    sys.stdout.write("Starting %i seconds of testing...\n" % experiment_duration)

    get_ports()
    connect_riots()

    close_connections()

if __name__ == "__main__":
    main()