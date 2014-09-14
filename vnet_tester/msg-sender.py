#!/usr/bin/env python

from __future__ import print_function

import subprocess
import sys
import json


print(sys.argv[1])
config = json.loads(sys.argv[1])
ports = config.keys()


print("\nSelect a node to send message to:\n")
for index, port in enumerate(ports):
    print("\t[%d] Port: %s IP: %s" % (index, port, config[port]))

def run(cmd):
    proc = subprocess.Popen(cmd, stdout=sys.stdout, shell=True)
    (out, err) = proc.communicate()
    return out

def send_msg(source, target):
    if source == target:
        print("Node cannot send message to itself")
        return

    if source >= len(ports):
        print("Source node (%d) does not exist :<" % (source))
        return

    if target >= len(ports):
        print("Target node (%d) does not exist :<" % (target))
        return

    s_port = ports[source]
    s_ip = config[s_port]

    t_port = ports[target]
    t_ip = config[t_port]

    print("Sending from node with IP %s (%s) to IP %s (%s)" % (s_ip, s_port, t_ip, t_port))

    # -q 5 means that netcat will stop listening after 5 seconds
    netcat_cmd = "echo 'send_data %s' | nc -q 5 localhost %s" % (t_ip, s_port)
    print("Netcat command: %s" % (netcat_cmd))
    run([netcat_cmd])




while True:
    try:
        source = int(raw_input("\nWhich source node? "))
        target = int(raw_input("\nWhich target node? "))
        send_msg(source, target)
    except ValueError:
        print("Please input a node number!")





