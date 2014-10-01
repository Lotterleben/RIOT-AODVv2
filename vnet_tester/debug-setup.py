#!/usr/bin/env python

from __future__ import print_function

import subprocess
import sys
import re
import json
import os

def run(cmd):
    proc = subprocess.Popen(cmd, stdout=subprocess.PIPE, shell=True)
    (out, err) = proc.communicate()
    return out


# Get IP/Port mapping generated by aodv_test.py
try:
    ip_port_info_f = open('ip_port.info', 'r')
except IOError:
    print("Couldn't find ip_port.info file. Did you run aodv_test.py?", file=sys.stderr)
    sys.exit(1)

ip_port_info = json.loads(ip_port_info_f.read())



# Get AODVv2 processes and create a dictionary mapping their ports to process ids
ps_aux_regex = re.compile("[^\s]+\s+([0-9]+).*-t ([0-9]+)")
procs = run(['ps aux | grep aodvv2.*elf ']).split("\n")
procs = [ps_aux_regex.search(p) for p in procs
            if ps_aux_regex.search(p) != None]
procs = dict([(p.group(2), p.group(1)) for p in procs])
pids = procs.values()


# # {
# #  "4715": "fe80::ff:fe00:1ea8",
# #  "4716": "fe80::ff:fe00:1eab",
# #  "4717": "fe80::ff:fe00:1eae"
# # }
# #
# {"4715": "fe80::ff:fe00:1ea8", "4716": "fe80::ff:fe00:1eab", "4717": "fe80::ff:fe00:1eae"}



if len(procs) != 4:
    print('Sorry, this script only supports a 4x1 line topology but %d nodes are running.' % (len(procs)), file=sys.stderr)
    print('Nodes (port, process id):')
    print(procs)
    sys.exit(1)




tmux_cmd = """
    tmux new-session -d -s foo "python msg-sender.py \\"%(ip_port_info)s\\"; bash"
    tmux select-window -t foo:0
    tmux split-window -h -t 1 'sudo gdb program %(pid1)s -x %(cwd)s/default_breakpoints; bash'
    tmux split-window -h 'sudo gdb program %(pid2)s -x %(cwd)s/default_breakpoints; bash'
    tmux split-window -v -t 1 'sudo gdb program %(pid3)s -x %(cwd)s/default_breakpoints; bash'
    tmux split-window -v -t 2 'sudo gdb program %(pid4)s -x %(cwd)s/default_breakpoints; bash'
    tmux -2 attach-session -t foo
""" % {
    'ip_port_info': json.dumps(ip_port_info).replace('"', '\\\\\\"'),
    'pid1': pids[0],
    'pid2': pids[1],
    'pid3': pids[2],
    'pid4': pids[3],
    'cwd': os.getcwd()

}
print(tmux_cmd)
run([tmux_cmd])