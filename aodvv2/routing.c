/*
 * Cobbled-together routing table.
 * This is neither efficient nor elegant, but until RIOT gets their own native
 * RT, this will have to do.
 */

#include <string.h>

#include "routing.h" 

static aodvv2_routing_entry_t routing_table[AODVV2_MAX_ROUTING_ENTRIES];

ipv6_addr_t* get_next_hop(ipv6_addr_t* addr){

}

aodvv2_routing_entry_t* get_routing_entry(ipv6_addr_t* addr)
{

}

int update_routing_entry()
{

}

int delete_routing_entry(ipv6_addr_t* addr)
{

}

int clear_routingtable(void)
{

}