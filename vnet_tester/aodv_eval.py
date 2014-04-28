import argparse
import traceback
import sys
import os
import re
from bs4 import BeautifulSoup, NavigableString

import numpy as np
import matplotlib.pyplot as plt

import pretty_plots as pp

RFC5444_MSGTYPE_RREQ = "10"
RFC5444_MSGTYPE_RREP = "11"
RFC5444_MSGTYPE_RERR = "12"

RFC5444_MSGTLV_ORIGSEQNUM = "0"
RFC5444_MSGTLV_TARGSEQNUM = "1"
RFC5444_MSGTLV_UNREACHABLE_NODE_SEQNUM = "2"
RFC5444_MSGTLV_METRIC = "3"

DISCOVERY_ATTEMPTS_MAX = 3

working_dir = "./dumps/"
packets = []
all_ips = set()

def pcap_to_xml(pcap_file_str):
    global working_dir, xml_file_location

    print "converting to xml..."

    # make sure we have a directory to operate in
    if (not os.path.exists(working_dir)):
        os.makedirs(working_dir)

    xml_file_location = working_dir + pcap_file_str.split("/")[-1].split(".")[0] + ".xml"
    
    # make pcap python-readable by converting it to xml, store in file
    if (os.path.isfile(xml_file_location)):
        os.remove(xml_file_location)

    os.system("tshark -r %s -V -Y ipv6 -T pdml >> %s" %(pcap_file_str, xml_file_location))

    return xml_file_location

def store_pcap(pkt):
    ip_header = pkt.find(attrs = {"name":"ipv6"})
    src_addr = ip_header.find(attrs = {"name":"ipv6.src"})["show"]
    dst_addr = ip_header.find(attrs = {"name":"ipv6.dst"})["show"]
    data = ""

    packetbb = pkt.find(attrs = {"name":"packetbb"})
    if (packetbb):
        data = store_pktbb(packetbb)
    else:
        payload = pkt.find(attrs = {"name": "data.data"})
        if (payload is None):
            # found an icmp packet or so
            return
        data = payload["show"]

    packet = {"src": src_addr, "dst": dst_addr, "data": data}
    packets.append(packet)

def store_pktbb(packetbb):
    pkt = {}
    orignode = {}
    targnode = {}

    header = packetbb.find(attrs = {"name":"packetbb.msg.header"})
    addrblock = packetbb.find(attrs = {"name":"packetbb.msg.addr"})
    tlvblock = addrblock.find_all(attrs = {"name":"packetbb.tlv"})

    msg_type = header.find(attrs = {"name":"packetbb.msg.type"})["show"]
    hop_limit = header.find(attrs = {"name":"packetbb.msg.hoplimit"})["show"]

    addresses = addrblock.find_all(attrs = {"name":"packetbb.msg.addr.value6"})

    pkt["type"] = msg_type

    if((msg_type == RFC5444_MSGTYPE_RREQ) or (msg_type == RFC5444_MSGTYPE_RREP)):
        # there is no guarantee that the order is always right but I'm running out of time
        orignode["addr"] = addresses[0]["show"]
        targnode["addr"] = addresses[1]["show"]

        # store IPs for future use
        all_ips.add(orignode["addr"])

        for tlv in tlvblock:
            # whatever
            if (not isinstance(tlv, NavigableString)):
                tlv_indexstart = tlv.find(attrs = {"name":"packetbb.tlv.indexstart"})["show"]
                tlv_type = tlv.find(attrs = {"name":"packetbb.tlv.type"})["show"]
                tlv_value = tlv.find(attrs = {"name":"packetbb.tlv.value"})["value"]
                
                if (tlv_indexstart == "0"):
                    node = orignode
                elif (tlv_indexstart == "1"):
                    node = targnode      

                if ((tlv_type == RFC5444_MSGTLV_ORIGSEQNUM) or (tlv_type == RFC5444_MSGTLV_TARGSEQNUM)):
                    node["seqnum"] = tlv_value

                elif (tlv_type == RFC5444_MSGTLV_METRIC):
                    node["metric"] = tlv_value

        pkt["orignode"] = orignode
        pkt["targnode"] = targnode

    elif(msg_type == RFC5444_MSGTYPE_RERR):
        unreachable_nodes = []

        addresses = addrblock.find_all(attrs = {"name":"packetbb.msg.addr.value6"})
        tlvs = addrblock.find_all()
        addr_index = 0

        # TODO test this
        for addr in addresses:
            # format: {ip: seqnum}
            unreachable_nodes.append({"addr":addr["show"]})
        
        for tlv in tlvblock:
            tlv_value = tlv.find(attrs = {"name":"packetbb.tlv.value"})["value"]
            unreachable_nodes[addr_index]["seqnum"] = tlv_value # na ob das gut geht-...
            addr_index += 1

        pkt["unreachable_nodes"] = unreachable_nodes

    return pkt

def evaluate_pcap():
    global packets
    for ip in all_ips:
        my_discoveries = [pkt for pkt in packets if ("orignode" in pkt["data"]) and (ip in pkt["data"]["orignode"]["addr"])]
        #my_discoveries = [pkt for pkt in packets if (ip in pkt["src"])]
        
        # technically, we can't assume that the RREP actually survived its last hop, 
        # but this should at least provide an educated guess
        received_rreps = [pkt for pkt in my_discoveries if (RFC5444_MSGTYPE_RREP in pkt["data"]["type"]) and (ip in pkt["dst"])]

        #print "discoveries of ", ip, ": ", my_discoveries, "\n4"
        print len(received_rreps), "RREPs to ", ip, ":", received_rreps

def handle_capture(xml_file_location):
    print "handling capture..."

    xml_file = open(xml_file_location, "r")
    soup = BeautifulSoup(xml_file, "xml")

    pcaps = soup.find_all("packet")
    for pkt in pcaps:
        store_pcap(pkt)

    evaluate_pcap()

def handle_logfile(log_file_location):
    if (not os.path.isfile(log_file_location)):    
        print "Couldn't find log file. Aborting."
        return

        # todo update me
    results = count_successes(log_file_location)
    print results

    successes = (results["discoveries"]["success"], results["transmissions"]["success"])
    failures = ((results["rrep_fail"], 0), (results["discoveries"]["fail"], results["transmissions"]["fail"]))

    # TODO discoveries within timeout into plot
    pp.plot_bars(("successful", "failed at RREP", "failed"),("Route Discoveries", "Transmissions"), successes, failures)

def count_successes(log_file_location):
    print "counting successful route discoveries and transmissions..."
    logfile = open(log_file_location)
    rreqs_sent = {}
    rreqs_arrived = 0
    rreqs_total = 0
    rreqs_buf = 0

    node_switch = re.compile ("(.*) {(.*)} send_data to (.*)") 
    rrep_received = False

    discoveries = {"success" : 0, "fail" : 0}
    discoveries_within_timeout = 0
    transmissions_total = 0
    transmissions = {"success" : 0, "fail" : 0}

    # (here's to hoping I'll never change that debug output...)
    for line in logfile:
        # TODO: sowas (aber mit ip des senders..) parsen [demo]   sending packet of %i bytes towards %s...\n"
        if ("[demo]   sending packet" in line):
            transmissions_total += 1
        elif ("[demo]   UDP packet received from" in line):
            transmissions["success"] += 1
        # look for successful route discovery
        elif ("[aodvv2] originating RREQ" in line):  
            rreqs_buf += 1
            '''
            since we're currently only sending 1 RREQ per transmission anyway, we can skip this
            ip = line.split(" ")[4].strip()[:-1]
            try:
                # found (possibly redundant) RREQ
                if (rreqs_sent[ip] < DISCOVERY_ATTEMPTS_MAX):
                    rreqs_sent[ip] += 1
                # rreq has been sent without success last time; reset counter
                else:
                    rreqs_sent[ip] = 1
                    discoveries["fail"] += 1
            except:
                # this is the first rreq to ip
                rreqs_sent[ip] = 0

            print rreqs_sent
            '''

        elif("[aodvv2] TargNode is in client list, sending RREP" in line):
            rreqs_arrived +=1

        elif ("This is my RREP." in line):
            ip = line.split(" ")[1].strip()[:-1]
            #print "found ip: ", ip #, line
            discoveries["success"] += 1
            rrep_received = True

        # reached log entries of another node
        if (node_switch.match(line)):
            print line
            if (rreqs_buf > 0):
                rreqs_total += 1
                if (rreqs_total <= 3 and rrep_received is True):
                    discoveries_within_timeout += 1
                rreqs_buf = 0
                rrep_received = False


    transmissions["fail"] = transmissions_total - transmissions["success"]
    discoveries["fail"] = rreqs_total - discoveries["success"]
    rrep_fails = discoveries["fail"] - rreqs_arrived 

    return {"discoveries" : discoveries, "transmissions" : transmissions, 
    "discoveries within timeout" : discoveries_within_timeout, "rrep_fail": rrep_fails}

def main():
    pcap_file_str = ""
    
    parser = argparse.ArgumentParser(description='evaluate traffic generated by auto_test')
    parser.add_argument('-p','--pcap', type=str, help='pcap file to evaluate')
    parser.add_argument('-l', '--log', type=str, help='aodv_test log file to evaluate')

    args = parser.parse_args()
    if (args.pcap):
        print "evaluating pcap..."
        pcap_file_str = args.pcap

        xml_file_location  = pcap_to_xml(pcap_file_str)
        handle_capture(xml_file_location)

    if (args.log):
        print "evaluating log..."
        handle_logfile(args.log)

    #print pktbb_packets

if __name__ == "__main__":
    main()