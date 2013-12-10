/* No real/proper tests, just some methods to check rotine things */

#import "include/aodvv2.h"
#import "routing.h"
#include "common/netaddr.h"

void test_rt(void)
{
    timex_t now, validity_t;
    uint8_t success;
    ipv6_addr_t address, next_hop;
    char addr_str[IPV6_MAX_ADDR_STR_LEN];

    init_routingtable();

    /* init data */
    ipv6_addr_set_loopback_addr(&address);
    ipv6_addr_set_all_nodes_addr(&next_hop);

    rtc_time(&now);
    validity_t = timex_set(AODVV2_ROUTE_VALIDITY_TIME, 0);

    struct aodvv2_routing_entry_t entry_1 = {
        .address = address,
        .prefixLength = 5,
        .seqNum = 6,
        .nextHopAddress = next_hop,
        .lastUsed = now,
        .expirationTime = validity_t,
        .broken = false,
        .metricType = 0,
        .metric = 20,
        .state = ROUTE_STATE_IDLE
    };

    rtc_time(&now);
    validity_t = timex_set(AODVV2_ROUTE_VALIDITY_TIME, 0);

    struct aodvv2_routing_entry_t entry_2 = {
        .address = next_hop,
        .prefixLength = 5,
        .seqNum = 12,
        .nextHopAddress = next_hop,
        .lastUsed = now,
        .expirationTime = validity_t,
        .broken = false,
        .metricType = 0,
        .metric = 10,
        .state = ROUTE_STATE_ACTIVE
    };

    /* start testing */
    print_rt();
    printf("Adding first entry: %s ...\n", ipv6_addr_to_str(addr_str, &address));
    //add_routing_entry(&address, 1, 2, &next_hop, 0, 13, 0, ROUTE_STATE_IDLE);
    add_routing_entry(&entry_1);
    print_rt();
    printf("Adding second entry: %s ...\n", ipv6_addr_to_str(addr_str, &next_hop));
    add_routing_entry(&entry_2);
    print_rt();
    printf("Deleting first entry: %s ...\n", ipv6_addr_to_str(addr_str, & address));
    delete_routing_entry(&address);
    print_rt();
    printf("getting next hop of second entry:\n");
    printf("\t%s\n",ipv6_addr_to_str(addr_str, get_next_hop(&next_hop)));
}