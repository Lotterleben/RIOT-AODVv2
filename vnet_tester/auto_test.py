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

riots = {}
riots_lock = threading.Lock()
sockets = []
sockets_lock = threading.Lock()
ports_local_path = "../../riot/desvirt_mehlis/ports.list" # TODO properly
experiment_duration = 200
max_silence_interval = 20

def get_ports():
    with open(ports_local_path, 'r') as f:
        ports_local = f.readlines()

        for port_string in ports_local:
            port = port_string.split(",")[2].rstrip('\n')
            riots[port] = ""

def get_node_ip(data):
    ips = data.split("\n");
    return ips[1]

def get_shell_output(sock):
    # read IP data until ">" marks termination of the shell output
    data = chunk = ""

    while (not(">" in data)):
        chunk = sock.recv(4096)
        data += chunk
    return data

def connect_riots():
    for port, ip in riots.iteritems():
        start_new_thread(test_sender_thread,(port,))
        # make sure the main thread isn't killed before we initialize our sockets
        time.sleep(2) 
    
    time.sleep(experiment_duration) 

def test_sender_thread(port):
    sys.stdout.write("Port: %s\n" % port)

    data = my_ip = random_neighbor = ""
    thread_id = threading.currentThread().name

    sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    with sockets_lock:
        sockets.append(sock)

    try:
        sock.connect(("127.0.0.1 ", int(port)))
        sock.sendall("ip\n")

        #get all IP addresses of this RIOT
        data = get_shell_output(sock)

        # check for errors (superduper professionally)
        if ("shell: command not found" in data):
            sys.stdout.write("Hickup while retrieving IPs. Exiting. Please try again.\n")
            sys.exit()

        # get relevant IP address
        my_ip = get_node_ip(data)
        #sys.stdout.write("{%s} IP: %s\n" % (thread_id, my_ip))
        logging.debug("{%s} IP: %s\n" % (thread_id, my_ip))


        with riots_lock:
            sys.stdout.write("adding node with IP %s to riots...\n" % my_ip)
            riots[port] = my_ip

        #for i in range (0,5):
        while (True):
            #wait for a little while
            some_time = random.randint(1, max_silence_interval)
            time.sleep(some_time)
            
            # pick random node
            with riots_lock:
                random_port = random.choice(riots.keys())
                random_neighbor = riots[random_port]
                #sys.stdout.write("new random neighbor:%s\n" % random_neighbor)

            # send data. just skip the uninitialized ones
            if ((random_neighbor != my_ip) and (random_neighbor != "")):
                sys.stdout.write("%s Say hi to   %s\n\n" % (port, random_neighbor))
                sock.sendall("send %s hello\n" % random_neighbor)

                logging.debug("{%s} %s\n%s" % (thread_id, my_ip, get_shell_output(sock)))

    except:
        traceback.print_exc()
        sys.exit()

# kill tcp connections on SIGINT
def signal_handler(signal, frame):
        print "\nCleaning up..."

        for socket in sockets:
            with sockets_lock:
                socket.close
                print "socket",socket.fileno(),"closed."

        print "done"
        sys.exit(0)

if __name__ == "__main__":
    timestamp = time.time()
    date = datetime.datetime.fromtimestamp(timestamp).strftime('%d-%m-%Y %H:%M:%S')
    logfile_name = "logs/auto_test "+date+".log"
    
    logging.basicConfig(filename=logfile_name, level=logging.DEBUG, format='%(asctime)s %(message)s', datefmt='%d-%m-%Y %H:%M:%S')

    signal.signal(signal.SIGINT, signal_handler)

    get_ports()
    connect_riots()