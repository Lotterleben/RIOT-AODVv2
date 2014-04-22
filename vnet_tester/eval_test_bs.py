import argparse
import traceback
import sys
import os
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

    if((msg_type == RFC5444_MSGTYPE_RREQ) or (msg_type == RFC5444_MSGTYPE_RREP)):
        # there is no guarantee that the order is always right but I'm running out of time
        orignode["addr"] = addresses[0]["show"]
        targnode["addr"] = addresses[1]["show"]

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

    #print "\t", pkt
    return pkt

def store_data(pkt):
    ip_header = pkt.find(attrs = {"name":"ipv6"})
    src_addr = ip_header.find(attrs = {"name":"ipv6.src"})["show"]
    dst_addr = ip_header.find(attrs = {"name":"ipv6.dst"})["show"]
    data = ""

    #data = pkt.find(attrs = {"name":"ipv6.dst"})["show"]
    packetbb = pkt.find(attrs = {"name":"packetbb"})
    if (packetbb):
        #print "i can haz packetbb", type(packetbb), type(ip_header)
        data = store_pktbb(packetbb)

    foo = {"src": src_addr, "dst": dst_addr, "data": data}

    print foo


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


def handle_capture(xml_file_location):
    print "handling capture..."

    xml_file = open(xml_file_location, "r")
    soup = BeautifulSoup(xml_file, "xml")

    ''' commented out for debugging
    packetbb_pkts = soup.find_all(attrs = {"name":"packetbb"})
    for pkt in packetbb_pkts:
        store_pktbb(pkt)

    data_pkts = soup.find_all(attrs = {"name":"ipv6"})
    print data_pkts
    for pkt in data_pkts:
        store_data(pkt)
    '''

    packets = soup.find_all("packet")
    for pkt in packets:
        store_data(pkt)


def handle_logfile(log_file_location):
    if (not os.path.isfile(log_file_location)):    
        print "Couldn't find log file. Aborting."
        return

        # todo update me
    results = count_successes(log_file_location)
    print results

    successes = (results["discoveries"]["success"], results["transmissions"]["success"])
    failures = (results["discoveries"]["fail"], results["transmissions"]["fail"])

    # TODO discoveries within timeout into plot
    pp.plot_bars(("successful", "failed"),("Route Discoveries", "Transmissions"), successes, failures)


def count_successes(log_file_location):
    print "counting successful route discoveries and transmissions..."
    logfile = open(log_file_location)
    rreqs_sent = {}
    rreqs_total = 0

    discoveries = {"success" : 0, "fail" : 0}
    discoveries_within_timeout = 0
    transmissions = {"success" : 0, "fail" : 0}

    for line in logfile:
        # TODO: sowas (aber mit ip des senders..) partsen [demo]   sending packet of %i bytes towards %s...\n"

        # look for successful transmission
        if ("[demo]  Success sending" in line):
            transmissions["success"] += 1
        elif ("[demo]  Error sending" in line):
            transmissions["fail"] += 1

        # look for successful route discovery
        # (here's to hoping I'll never change that debug output...)
        elif ("[aodvv2] originating RREQ" in line):
            # TODO ip rausfiltern
            ip = line.split(" ")[4].strip()[:-1]
            print "originating ip: ", ip #, line
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

        # if rrep found:
        elif ("This is my RREP." in line):
            ip = line.split(" ")[1].strip()[:-1]
            print "found ip: ", ip #, line
            discoveries["success"] += 1
            if (rreqs_sent[ip] < 3 ):
                print "OMG!!! Success within timeout!"
                discoveries_within_timeout += 1
            rreqs_sent[ip] = 0

    return {"discoveries" : discoveries, "transmissions" : transmissions, "discoveries_within_timeout" : discoveries_within_timeout}


def plot_bars(labels, values):

    x_loc = np.arange(2) # the x locations for the groups

    fig, ax = plt.subplots()
    bars = plt.bar(x_loc, values, color='b',  align='center', alpha=0.4)

    plt.xticks(x_loc, labels)
    plt.show()

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