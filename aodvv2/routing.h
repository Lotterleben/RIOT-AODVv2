#ifndef ROUTING_H_
#define ROUTING_H_

#include <string.h>

#include "ipv6.h"
#include "common/netaddr.h"
#include "constants.h"

/*
 * A route table entry (i.e., a route) may be in one of the following
   states:
 */
enum aodvv2_routing_states {
    ROUTE_STATE_ACTIVE,
    ROUTE_STATE_IDLE,
    ROUTE_STATE_EXPIRED,
    ROUTE_STATE_BROKEN,
    ROUTE_STATE_TIMED
};

/* contains all fields of a routing table entry */
struct aodvv2_routing_entry_t {
    struct netaddr addr; 
    uint8_t seqnum;
    struct netaddr nextHopAddr;
    timex_t lastUsed;
    timex_t expirationTime;
    uint8_t metricType;
    uint8_t metric;
    uint8_t state; /* see aodvv2_routing_states */
};

void routingtable_init(void);
/* returns NULL if addr is not in routing table */
struct netaddr* routingtable_get_next_hop(struct netaddr* addr, uint8_t metricType);
void routingtable_add_entry(struct aodvv2_routing_entry_t* entry);
struct aodvv2_routing_entry_t* routingtable_get_entry(struct netaddr* addr, uint8_t metricType);
void routingtable_delete_entry(struct netaddr* addr, uint8_t metricType);
void print_routingtable(void);
void print_routingtable_entry(struct aodvv2_routing_entry_t* rt_entry);
/* I'm really sorry about my naming choice here. */
void routingtable_break_and_get_all_hopping_over(struct netaddr* hop, struct unreachable_node unreachable_nodes[], int* len);

#endif /* ROUTING_H_ */ 
