/*
 * Cobbled-together routing table.
 * This is neither efficient nor elegant, but until RIOT gets their own native
 * RT, this will have to do.
 */
#include <string.h>

#include "ipv6.h"

#include "common/netaddr.h"

#include "include/aodvv2.h"

#ifndef ROUTING_H_
#define ROUTING_H_

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
    struct netaddr address; 
    uint8_t prefixlen; //should be long enough, no?
    uint8_t seqNum;
    struct netaddr nextHopAddress;
    timex_t lastUsed; // use timer thingy for this?
    timex_t expirationTime;
    bool broken;
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
#endif /* ROUTING_H_ */ 
