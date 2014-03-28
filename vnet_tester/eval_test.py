
import argparse
from scapy.all import *
import traceback
import sys

pcap_file_str = ""

def parse_pcap():
    try:
        pkts = rdpcap(pcap_file_str)
    except:
        traceback.print_exc()
        sys.exit()
   
    for pkt in pkts:
        print pkt.show()


def main():
    global pcap_file_str
    
    parser = argparse.ArgumentParser(description='evaluate traffic generated by auto_test')
    parser.add_argument('-f','--file', type = str, help='pcap file to evaluate')

    args = parser.parse_args()
    pcap_file_str = args.file

    parse_pcap()

if __name__ == "__main__":
    main()