#ifndef UTILS_H_
#define UTILS_H_
#include <stdio.h>

#include "ipv6.h"

#include "common/netaddr.h"

#include "constants.h"
#include "seqnum.h"

#define AODVV2_MAX_CLIENTS 1    // multiple clients are currently not supported.
#define AODVV2_RREQ_BUF 128     // should be enough for now...
#define AODVV2_RREQ_WAIT_TIME 2 // seconds

struct aodvv2_rreq_entry {
    struct netaddr origNode;
    struct netaddr targNode;
    uint8_t metricType;
    uint8_t metric;
    uint16_t seqnum;
    timex_t timestamp;
};

/* Section 5.3.: Client Table functionality */
void clienttable_init(void);
void clienttable_add_client(struct netaddr* addr);
bool clienttable_is_client(struct netaddr* addr);
void clienttable_delete_client(struct netaddr* addr);

/* Sections 5.7. and 7.6.: RREQ table functionality */
void rreqtable_init(void);
bool rreqtable_is_redundant(struct aodvv2_packet_data* packet_data);

/* various utilities */
void ipv6_addr_t_to_netaddr(ipv6_addr_t* src, struct netaddr* dst);
void netaddr_to_ipv6_addr_t(struct netaddr* src, ipv6_addr_t* dst);

#endif /* UTILS_H_ */
