/* No real/proper tests, just some methods to check rotine things */

#import "routing.h"
#include "common/netaddr.h"

void test_rt(void)
{
    
}

void print_rt_entry(aodvv2_routing_entry_t* rt_entry, int index)
{   
    struct netaddr_str nbuf;

    printf("routing table entry at %i:\n", index );
    printf("\t address: %s\n", netaddr_to_string(&nbuf, &rt_entry->address));
    printf("\t prefixLength: %i\n", rt_entry->prefixLength);
    printf("\t seqNum:\n", rt_entry->seqNum);
    printf("\t nextHopAddress: %s\n", netaddr_to_string(&nbuf, &rt_entry->nextHopAddress));
    printf("\t lastUsed: %i\n", rt_entry->lastUsed);
    printf("\t expirationTime: %i\n", rt_entry->expirationTime);
    printf("\t broken: %d\n", rt_entry->broken);
    printf("\t metricType: %i\n", rt_entry->metricType);
    printf("\t metric: %i\n", rt_entry->metric);
    printf("\t state: \n", rt_entry->state);
}