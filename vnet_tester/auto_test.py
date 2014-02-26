import socket
import time
import traceback
import sys
from thread import start_new_thread
import threading
import random

riots = {}
riots_lock = threading.Lock()
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

def connect_riots():
    for port, ip in riots.iteritems():
        start_new_thread(test_sender_thread,(port,))
        # make sure the main thread isn't killed before we initialize our sockets
        time.sleep(2) 
    
    time.sleep(experiment_duration) 

def test_sender_thread(port):
    sys.stdout.write("Port: %s\n" % port)
    
    sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    data = ""
    my_ip = ""
    random_neighbor = ""

    try:
        sock.connect(("127.0.0.1 ", int(port)))
        sock.sendall("ip\n")

        # read IP data until ">" marks termination of the shell output
        while (not(">" in data)):
            chunk = sock.recv(4096)
            data += chunk

        # check for errors (superduper professionally)
        if ("shell: command not found" in data):
            sys.stdout.write("Hickup while retrieving IPs. Exiting. Please try again.\n")
            sys.exit()
        my_ip = get_node_ip(data)
        sys.stdout.write("IP: %s\n" % my_ip)

        with riots_lock:
            sys.stdout.write("adding node to riots...\n")
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

    except:
        traceback.print_exc()
        sys.exit()

if __name__ == "__main__":
    initialized_riots = 0
    get_ports()
    print riots
    connect_riots()