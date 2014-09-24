#!/usr/bin/env python

from __future__ import print_function

import subprocess
import sys
import json


print(sys.argv[1])
config = json.loads(sys.argv[1])
ports = config.keys()


print("\nAvailable nodes:\n")
for index, port in enumerate(ports):
    print("\t[%d] Port: %s IP: %s" % (index, port, config[port]))

def run(cmd, s_port):
    # -q 5 means that netcat will stop listening after 5 seconds
    netcat_cmd = "echo '%s' | nc -q 5 localhost %s" % (cmd, s_port)
    print("Netcat command: %s" % (netcat_cmd))

    proc = subprocess.Popen([netcat_cmd], stdout=sys.stdout, shell=True)
    (out, err) = proc.communicate()
    return out

def send_data(source, target):
    if source == target:
        print("Error: Node cannot send message to itself")
        return

    if source >= len(ports):
        print("Error: Source node (%d) does not exist :<" % (source))
        return

    if target >= len(ports):
        print("Error: Target node (%d) does not exist :<" % (target))
        return

    s_port = ports[source]
    s_ip = config[s_port]

    t_port = ports[target]
    t_ip = config[t_port]

    print("Sending data from node with IP %s (%s) to IP %s (%s)" % (s_ip, s_port, t_ip, t_port))

    cmd = "send_data %s" % (t_ip)
    run(cmd, s_port)

def handle_cmd(cmd):
    parts = cmd.split(" ")
    try:
        node_number = int(parts[0])
    except ValueError:
        print("Error: node number (1,2,3,...) not specified")
        return

    node_port = ports[node_number]
    node_ip = config[node_port]

    cmd = parts[1]

    if (cmd == "print_rt"):
        run(cmd, node_port)

    elif (cmd == "send_data"):
        try:
            target_number = int(parts[2])
        except:
            print("Error: target node number (1,2,3,...) not specified")
            return
        send_data(node_number, target_number)

    elif (cmd == "send"):
        try:
            target_number = int(parts[2])
            parts[3]
        except ValueError:
            print("Usage: <node number> send <destination node number> <message>")
            return
        target_port = ports[target_number]
        target_ip = config[target_port]
        msg = parts[3]

        print("Sending message %s from node with IP %s (%s) to IP %s (%s)" % (msg, node_ip, node_port, target_ip, target_port))

        cmd = "send %s %s" % (target_ip, msg)
        run(cmd, node_port)

    else:
        print("unknown command: ", cmd)

while True:
    try:
        #source = int(raw_input("\nWhich source node? "))
        #target = int(raw_input("\nWhich target node? "))
        #send_msg(source, target)
        cmd = raw_input("\nenter command to be executed on node: <node number> <command>\n")
        handle_cmd(cmd)
    except ValueError:
        print("Faulty input!")





