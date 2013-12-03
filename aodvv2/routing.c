/*
 * Cobbled-together routing table.
 * This is neither efficient nor elegant, but until RIOT gets their own native
 * RT, this will have to do.
 */

#include <string.h>

#include "routing.h" 

static aodvv2_routing_entry_t routing_table[AODVV2_MAX_ROUTING_ENTRIES];

void init_routingtable(void)
{
    for (uint8_t i = 0; i < AODVV2_MAX_ROUTING_ENTRIES; i++) {
        memset(&routing_table[i], 0, sizeof(routing_table[i]));
    }
}

ipv6_addr_t* get_next_hop(ipv6_addr_t* addr)
{
    for (uint8_t i = 0; i < AODVV2_MAX_ROUTING_ENTRIES; i++) {
        if (rpl_equal_id(&routing_table[i].address, addr) && !(&routing_table[i].broken)) {
            return &routing_table[i].nextHopAddress;
        }
    }
    return NULL;
}

int add_routing_entry(aodvv2_routing_entry_t* rt_entry)
{
    /* only update if we don't already know the address
     * TODO: does this always make sense?
     */
    if(!(get_routing_entry(&rt_entry->address))){ // na ob das so stimmt...

    }
    return -1;
}

/*
 * retrieve pointer to a routing table entry. To edit, simply follow the pointer.
 */
aodvv2_routing_entry_t* get_routing_entry(ipv6_addr_t* addr)
{
    for (uint8_t i = 0; i < AODVV2_MAX_ROUTING_ENTRIES; i++) {
        if (ipv6_addr_is_equal(&routing_table[i].address, addr)) {
            return &routing_table[i];
        }
    }
    return NULL;
}

int delete_routing_entry(ipv6_addr_t* addr)
{
    for (uint8_t i = 0; i < AODVV2_MAX_ROUTING_ENTRIES; i++) {
        if (rpl_equal_id(&routing_table[i].address, addr)) {
            memset(&routing_table[i], 0, sizeof(routing_table[i]));
            return 0;
        }
    }
    return -1;
}