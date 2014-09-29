import argparse
import traceback
import sys
import os
import re
import pprint
import sys

from bs4 import BeautifulSoup, NavigableString

import numpy as np
import matplotlib.pyplot as plt

import pretty_plots as pp
import topology_viz as tv


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

    if ((msg_type == RFC5444_MSGTYPE_RREQ) or (msg_type == RFC5444_MSGTYPE_RREP)):
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

    elif (msg_type == RFC5444_MSGTYPE_RERR):
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
    num_received_rreps = 0
    num_discoveries = 0

    #rerrs = [pkt for pkt in packets if (RFC5444_MSGTYPE_RERR in pkt["data"]["type"])]

    for ip in all_ips:
        my_discoveries = [pkt for pkt in packets if ("orignode" in pkt["data"]) and (ip in pkt["data"]["orignode"]["addr"])]
        num_discoveries += len(my_discoveries)/3

        # technically, we can't assume that the RREP actually survived its last hop,
        # but this should at least provide an educated guess
        received_rreps = [pkt for pkt in my_discoveries if (RFC5444_MSGTYPE_RREP in pkt["data"]["type"]) and (ip in pkt["dst"])]
        num_received_rreps += len(received_rreps)

        #print "discoveries of ", ip, ": ", my_discoveries, "\n4"
        #print len(received_rreps), "RREPs to ", ip, ":", received_rreps

    return {"discoveries" : num_discoveries, "rrep received": num_received_rreps}

def handle_capture(xml_file_location):
    print "handling capture..."
    pp = pprint.PrettyPrinter(indent=2)

    xml_file = open(xml_file_location, "r")
    soup = BeautifulSoup(xml_file, "xml")

    pcaps = soup.find_all("packet")
    for pkt in pcaps:
        store_pcap(pkt)

    print "pcap evaluation results: "
    pp.pprint(packets)
    pcap_results = evaluate_pcap()
    print "number of received RREPs:", pcap_results["rrep received"]
    print "number of started discoveries:", pcap_results["discoveries"]

def handle_logfile(log_file_location):
    if (not os.path.isfile(log_file_location)):
        print "Couldn't find log file. Aborting."
        return

        # todo update me
    results = count_successes(log_file_location)
    print "results: ", results
    disc_within_to = results["discoveries within timeout"]

    successes = ((disc_within_to, 0),(results["discoveries"]["success"] - disc_within_to , results["transmissions"]["success"]))
    failures = ((results["rrep_fail"], 0), (results["discoveries"]["fail"] - results["rrep_fail"], results["transmissions"]["fail"]))

    #pp.plot_bars(("successful within timeout", "successful", "failed at RREP", "failed"),("Route Discoveries", "Transmissions"), successes, failures)
    #pp.plot_bars(("successful within timeout", "successful", "failed"),("Route Discoveries", "Transmissions"), successes, failures)
    pp.plot_bars(("successful", "failed"),("Route Discoveries", "Transmissions"), successes, failures)

'''
route discoveries are of the following form:
{
    "orignode": "::f00",
    "targnode": "::bar",
    "seqnums": [],
    "success": 0
}
transmissions are of the following form:
{
    "timestamp": "69:635643",
    "orignode": "::f00",
    "targnode": "::bar",
    "success": 0
}
'''
def count_successes(log_file_location):
    print "counting successful route discoveries and transmissions..."
    pp = pprint.PrettyPrinter(indent=2)

    logfile = open(log_file_location)
    route_discoveries = []
    transmissions = []
    curr_ip = ""
    curr_discovery = {"orignode": "", "targnode": ""}
    line_number = 0

    for line in logfile:
        line_number += 1

        # reached original topology info; convert it to graphviz-readable format and print
        if ("riots:" in line):
            info = line.split("riots: ")[1]
            print info
            print "graphviz:\n", tv.prep_graphviz(info)

        node_log_start = re.search(".* {Dummy-.*: (.*), \(.*\)\}", line) # TODO match more thoroughly

        if (node_log_start):
            curr_ip = node_log_start.groups()[0]
            print "setting current IP to:", curr_ip

        elif ("getting next hop for" in line): # TODO gegen getting next hop for
            # save old discovery (if not empty), record new one
            #if (curr_discovery):
            #    route_discoveries.append(curr_discovery)
            info = re.search("\[aodvv2\] aodv_get_next_hop\(\) (.*): getting next hop for (.*)", line).groups()
            orignode = info[0]
            targnode = info[1]
            if (orignode != curr_ip):
                print "ERROR: current IP ", curr_ip, "differs from orignode ", orignode

            print curr_discovery

            # only add new discovery if it is not a Route Discovery Retry
            if (not (curr_discovery["orignode"] == orignode and curr_discovery["targnode"] == targnode)):
                curr_discovery = {}

                curr_discovery["orignode"] = orignode
                curr_discovery["targnode"] = targnode
                curr_discovery["seqnums"] = []
                curr_discovery["success"] = 0
                curr_discovery["rreq_arrived"] = 0

                print "new discovery: ", curr_discovery

        # look for (successful) discoveries
        elif ("originating RREQ" in line):
            info = re.search("\[aodvv2\] originating RREQ with SeqNum (.*) towards (.*); updating RREQ table...", line).groups()
            seqnum = info[0]
            targnode = info[1]

            print line_number, ":", line
            print curr_discovery

            if (curr_discovery and targnode != curr_discovery.get("targnode")):
                print "ERROR: IP conflict between targnode of discovery: ", targnode, " and expected targnode: ", curr_discovery.get("targnode"), "in line ", line_number
                sys.exit()
                return # double tap!

            curr_discovery["seqnums"].append(seqnum)

            # TODO do this more elegantly
            # this is the first time this discovery is started -> add it to list of started discoveries
            if (len(curr_discovery["seqnums"]) == 1):
                route_discoveries.append(curr_discovery)

        # requested route is direct neighbor
        elif ("[ndp] found NC entry. Returning dest addr." in line):
            curr_discovery["success"] = 1

            #print line_number, ":", line
            #print "3", curr_discovery

        # delete logged attempt if no discovery has taken place because the route was already known
        elif ("found dest " in line):
            #print line_number, ":", line
            targnode = re.search(".*found dest (.*) in routing table", line).groups()[0]

            # a route towards dest already exists; the discovery has been successful before it even started.
            # thus, we can remove it from the discovery list, since there was nothing to discover.
            if (curr_discovery.get("targnode") == targnode and curr_discovery.get("seqnums") == []):
                #print "hbzd", curr_discovery
                curr_discovery["success"] = 1

        elif ("This is my RREP" in line):
            info = re.search(".* (.*):  This is my RREP \(SeqNum: (.*)\). We are done here, thanks (.*)!", line).groups()

            orignode = info[0]
            targnode = info[2]
            seqnum = info[1]

            # mark success in route_discoveries
            discovery = [disc for disc in route_discoveries if disc["orignode"] == orignode and disc["targnode"] == targnode] # and seqnum in disc["seqnums"]]
            if (len(discovery) > 1):
                print "WARNING: More than 1 suitable discovery found for line: \n", line

            discovery[0]["success"] = 1

        # look for (successful) transmissions
        # found initiation of new transmission
        if ("[demo]   sending packet" in line):
            [timestamp, targnode] = re.search("{(.*)}\[demo\]   sending packet of 15 bytes towards (.*)...", line).groups()
            transmission = { "timestamp": timestamp, "orignode": curr_ip, "targnode": targnode, "success": 0}
            transmissions.append(transmission)

        '''
        # found packet that arrived (-> successful transmission)
        if ("[demo]   UDP packet received" in line):
            orignode = re.search(".*\[demo\].*UDP packet received from (.*):.*", line).groups()[0]
            print line_number, ":", line
            print re.search(".*\[demo\].*UDP packet received from (.*):.*", line).groups()
            #print transmissions
            print "orignode: ", orignode, "targnode: ", targnode ,"\ncurr_ip: ", curr_ip
            print "transmissions:", transmissions
            print "matching transmissions", [t for t in transmissions if t["orignode"] == orignode and t["targnode"] == curr_ip]

            suitable_transmissions = [t for t in transmissions if t["orignode"] == orignode and t["targnode"] == curr_ip and t["success"] == 0]
            #print suitable_transmissions
            # doesn't matter which one exactly we mark as successful, just pick one.
            # TODO: herausfinden, warum es success: ... lines ohne suitable transmission gibt. doppelt gesendet?
            if (len(suitable_transmissions) > 1):
                suitable_transmissions[-1]["success"] = 1
            '''

    print "route_discoveries \n", pp.pprint(route_discoveries)
    #print "transmissions\n", pp.pprint(transmissions)

    successful_discoveries = [d for d in route_discoveries if d["success"] == 1]
    in_timeout_discoveries = [d for d in route_discoveries if len(d["seqnums"]) <3 and d["success"] == 1]
    successful_transmissions = [t for t in transmissions if t["success"] == 1]

    discovery_summary = {}
    discovery_summary["success"] = len (successful_discoveries)
    discovery_summary["fail"] = len(route_discoveries) - discovery_summary["success"]

    transmission_summary = {}
    transmission_summary["success"] = len(successful_transmissions)
    transmission_summary["fail"] = len(transmissions) - transmission_summary["success"]

    return {"discoveries" : discovery_summary, "transmissions" : transmission_summary,
    "discoveries within timeout" : len(in_timeout_discoveries), "rrep_fail": 0}

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