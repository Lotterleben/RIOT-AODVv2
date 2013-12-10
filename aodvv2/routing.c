/*
 * Cobbled-together routing table.
 * This is neither efficient nor elegant, but until RIOT gets their own native
 * RT, this will have to do.
 */

#include <string.h>

#include "routing.h" 
#include "include/aodvv2.h"
#include "common/netaddr.h"

static struct aodvv2_routing_entry_t routing_table[AODVV2_MAX_ROUTING_ENTRIES];

void init_routingtable(void)
{   
    for (uint8_t i = 0; i < AODVV2_MAX_ROUTING_ENTRIES; i++) {
        memset(&routing_table[i], 0, sizeof(routing_table[i]));
    }
}

ipv6_addr_t* get_next_hop(ipv6_addr_t* addr)
{
    struct aodvv2_routing_entry_t* entry = get_routing_entry(addr);
    return(&entry->nextHopAddress);
}

int add_routing_entry(ipv6_addr_t* address, uint8_t prefixLength, uint8_t seqNum,
    ipv6_addr_t* nextHopAddress, bool broken, uint8_t metricType, uint8_t metric, uint8_t state)
{
    timex_t now, validity_t;
    rtc_time(&now);
    validity_t = timex_set(AODVV2_ROUTE_VALIDITY_TIME, 0);

    /* only update if we don't already know the address
     * TODO: does this always make sense?
     */
    if (!(get_routing_entry(address))){ // na ob das so stimmt...
        /*find free spot in RT and place rt_entry there */
        for (uint8_t i = 0; i< AODVV2_MAX_ROUTING_ENTRIES; i++){
            /* seqNum can't be 0 so this has to work as an indicator of 
               an "empty" struct for now*/
            if (!routing_table[i].seqNum) {
                /* TODO: sanity check? */
                routing_table[i].address = *address;
                routing_table[i].prefixLength = prefixLength;
                routing_table[i].seqNum = seqNum;
                routing_table[i].nextHopAddress = *nextHopAddress;
                routing_table[i].lastUsed = now;
                routing_table[i].expirationTime = timex_add(now, validity_t);
                routing_table[i].broken = broken;
                routing_table[i].metricType = metricType;
                routing_table[i].metric = metric;
                routing_table[i].state = state;
                return 0;
            }
        }
        return -1;
    }
    return -1;
}

/*
 * retrieve pointer to a routing table entry. To edit, simply follow the pointer.
 */
struct aodvv2_routing_entry_t* get_routing_entry(ipv6_addr_t* addr)
{   
    for (uint8_t i = 0; i < AODVV2_MAX_ROUTING_ENTRIES; i++) {
        if (ipv6_addr_is_equal(&routing_table[i].address, addr)) {
            return &routing_table[i];
        }
    }
    return NULL;
}

void delete_routing_entry(ipv6_addr_t* addr)
{
    for (uint8_t i = 0; i < AODVV2_MAX_ROUTING_ENTRIES; i++) {
        if (ipv6_addr_is_equal(&routing_table[i].address, addr)) {
            memset(&routing_table[i], 0, sizeof(routing_table[i]));
            return;
        }
    }
}

void print_rt_entry(struct aodvv2_routing_entry_t* rt_entry, int index)
{   
    char addr_str[IPV6_MAX_ADDR_STR_LEN];


    printf("routing table entry at %i:\n", index );
    printf("\t address: %s\n", ipv6_addr_to_str(addr_str, &(rt_entry->address))); 
    printf("\t prefixLength: %i\n", rt_entry->prefixLength);
    printf("\t seqNum: %i\n", rt_entry->seqNum);
    printf("\t nextHopAddress: %s\n", ipv6_addr_to_str(addr_str, &(rt_entry->nextHopAddress)));
    printf("\t lastUsed: %i\n", rt_entry->lastUsed);
    printf("\t expirationTime: %i\n", rt_entry->expirationTime);
    printf("\t broken: %d\n", rt_entry->broken);
    printf("\t metricType: %i\n", rt_entry->metricType);
    printf("\t metric: %i\n", rt_entry->metric);
    printf("\t state: \n", rt_entry->state);
}

void print_rt(void)
{   
    struct aodvv2_routing_entry_t curr_entry;

    printf("===== BEGIN ROUTING TABLE ===================\n");
    for(int i = 0; i < AODVV2_MAX_ROUTING_ENTRIES; i++) {
        if (routing_table[i].seqNum) {
            print_rt_entry(&routing_table[i], i); // fuck it, I'm tired.
        }
    }
    printf("===== END ROUTING TABLE =====================\n");
}
