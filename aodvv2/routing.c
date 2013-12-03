/*
 * Cobbled-together routing table.
 * This is neither efficient nor elegant, but until RIOT gets their own native
 * RT, this will have to do.
 */

#include <string.h>

#include "routing.h" 

static aodvv2_routing_entry_t routing_table[AODVV2_MAX_ROUTING_ENTRIES];

ipv6_addr_t* get_next_hop(ipv6_addr_t* addr){
    /* TODO */
    return NULL;
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

}

int clear_routingtable(void)
{

}