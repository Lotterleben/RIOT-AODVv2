#include "common/netaddr.h"
#include "rfc5444/rfc5444_print.h"

#ifndef CONSTANTS_H_
#define CONSTANTS_H_

#define AODVV2_MAX_HOPCOUNT 20
#define AODVV2_MAX_ROUTING_ENTRIES 255
#define AODVV2_DEFAULT_METRIC_TYPE 3
#define AODVV2_ACTIVE_INTERVAL 5   // seconds
#define AODVV2_MAX_IDLETIME 200    // seconds 
#define MAX_SEQNUM_LIFETIME 300    // seconds
#define AODVV2_RIOT_PREFIXLEN 128  // TODO: invent better name

/* RFC5498 */
#define MANET_PORT  269

/* my multicast address */
struct netaddr na_mcast;

enum msg_type {
    RFC5444_MSGTYPE_RREQ = 10,
    RFC5444_MSGTYPE_RREP = 11,
};

enum tlv_type {
    RFC5444_MSGTLV_ORIGSEQNUM,
    RFC5444_MSGTLV_TARGSEQNUM,    
    RFC5444_MSGTLV_METRIC,
};

struct node_data {
    struct netaddr addr;
    uint8_t prefixlen;
    uint8_t metric;
    uint16_t seqNum;
};

/* contains all data contained in an aodvv2 packet */
struct aodvv2_packet_data {
    uint8_t hoplimit;
    struct netaddr sender;
    uint8_t metricType;
    struct node_data origNode;
    struct node_data targNode;
    timex_t timestamp;
};

#endif /* CONSTANTS_H_ */
