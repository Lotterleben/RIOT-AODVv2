import argparse
import traceback
import sys
import os
from bs4 import BeautifulSoup, NavigableString

RFC5444_MSGTYPE_RREQ = "10"
RFC5444_MSGTYPE_RREP = "11"
RFC5444_MSGTYPE_RERR = "12"

RFC5444_MSGTLV_ORIGSEQNUM = "0"
RFC5444_MSGTLV_TARGSEQNUM = "1"
RFC5444_MSGTLV_UNREACHABLE_NODE_SEQNUM = "2"
RFC5444_MSGTLV_METRIC = "3"

working_dir = "./dumps/"
packets = [] # store ALL the packets!

def store_pkt(packetbb):
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
                    ndoe = targnode      

                if ((tlv_type == RFC5444_MSGTLV_ORIGSEQNUM) or (tlv_type == RFC5444_MSGTLV_TARGSEQNUM)):
                    node["seqnum"] = tlv_value

                elif (tlv_type == RFC5444_MSGTLV_METRIC):
                    node["metric"] = tlv_value


        pkt["orignode"] = orignode
        pkt["targnode"] = targnode

    elif(msg_type == RFC5444_MSGTYPE_RERR):
        unreachable_nodes = []

        # TODO test this
        for addr in addrblock:
            pass

        pkt["unreachable_nodes"] = unreachable_nodes

    print "\t", pkt
    packets.append(pkt)



def handle_capture(xml_file_location):
    print "handling capture..."

    xml_file = open(xml_file_location, "r")
    soup = BeautifulSoup(xml_file, "xml")

    packetbb_pkts = soup.find_all(attrs = {"name":"packetbb"})
    for packetbb in packetbb_pkts:
        store_pkt(packetbb)

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
    os.system("tshark -r %s -V -Y packetbb -T pdml >> %s" %(pcap_file_str, xml_file_location))

    return xml_file_location


def main():
    pcap_file_str = ""
    
    parser = argparse.ArgumentParser(description='evaluate traffic generated by auto_test')
    parser.add_argument('-f','--file', type=str, help='pcap file to evaluate')

    args = parser.parse_args()
    pcap_file_str = args.file

    xml_file_location  = pcap_to_xml(pcap_file_str)
    handle_capture(xml_file_location)

    print packets

if __name__ == "__main__":
    main()