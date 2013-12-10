/*
 * Cobbled-together routing table.
 * This is neither efficient nor elegant, but until RIOT gets their own native
 * RT, this will have to do.
 */

#include "ipv6.h"

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
    ipv6_addr_t address; 
    uint8_t prefixLength; //should be long enough, no?
    uint8_t seqNum;
    ipv6_addr_t nextHopAddress;
    timex_t lastUsed; // use timer thingy for this?
    timex_t expirationTime; // siehe rtc stuff. TODO: richtige wahl? Doof zu inkrementieren...
    bool broken;
    uint8_t metricType;
    uint8_t metric;
    uint8_t state; /* see aodvv2_routing_states */
};

void init_routingtable(void);
ipv6_addr_t* get_next_hop(ipv6_addr_t* addr);
void add_routing_entry(struct aodvv2_routing_entry_t* entry);

/* 
OBACHT: sicher stellen dass immer nur 1 thread diesen entry
(andere entries sind unaffected, oder?) bearbeitet! wie stell ich das am 
elegantesten an?
*/
struct aodvv2_routing_entry_t* get_routing_entry(ipv6_addr_t* addr);
void delete_routing_entry(ipv6_addr_t* addr);
void print_rt(void);
char* ipv6_stringify(ipv6_addr_t* addr);