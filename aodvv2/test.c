/* No proper tests, just some methods to check routine things */

#include "include/aodvv2.h"
#include "routing.h"
#include "common/netaddr.h"

void test_rt(void)
{
    timex_t now, validity_t;
    uint8_t success;
    struct netaddr address, next_hop;
    struct netaddr_str nbuf;

    init_routingtable();

    /* init data */
    netaddr_from_string(&address, "::23");
    netaddr_from_string(&next_hop, "::42");

    rtc_time(&now);
    validity_t = timex_set(AODVV2_ACTIVE_INTERVAL + AODVV2_MAX_IDLETIME, 0); 

    struct aodvv2_routing_entry_t entry_1 = {
        .address = address,
        .prefixlen = 5,
        .seqNum = 6,
        .nextHopAddress = next_hop,
        .lastUsed = now,
        .expirationTime = timex_add(now, validity_t),
        .broken = false,
        .metricType = AODVV2_DEFAULT_METRIC_TYPE,
        .metric = 12,
        .state = ROUTE_STATE_IDLE
    };

    rtc_time(&now);

    struct aodvv2_routing_entry_t entry_2 = {
        .address = next_hop,
        .prefixlen = 5,
        .seqNum = 0, // illegal, but what the hell. for testing purposes. ahum.
        .nextHopAddress = next_hop,
        .lastUsed = now,
        .expirationTime = timex_add(now, validity_t),
        .broken = false,
        .metricType = AODVV2_DEFAULT_METRIC_TYPE,
        .metric = 13,
        .state = ROUTE_STATE_ACTIVE
    };

    /* start testing */
    print_rt();
    printf("Adding first entry: %s ...\n", netaddr_to_string(&nbuf, &address));
    //add_routing_entry(&address, 1, 2, &next_hop, 0, 13, 0, ROUTE_STATE_IDLE);
    add_routing_entry(&entry_1);
    print_rt();
    printf("Adding second entry: %s ...\n", netaddr_to_string(&nbuf, &next_hop));
    add_routing_entry(&entry_2);
    print_rt();
    printf("Deleting first entry: %s ...\n", netaddr_to_string(&nbuf, & address));
    delete_routing_entry(&address);
    print_rt();
    printf("getting next hop of second entry:\n");
    printf("\t%s\n", netaddr_to_string(&nbuf, get_next_hop(&next_hop)));
    printf("getting next hop of first (deleted) entry:\n");
    if (get_next_hop(&address) == NULL)
        printf("\tSuccess: address not in routing table, get_next_hop() returned NULL\n");
    else
        printf("\tSomething went wrong, get_next_hop() should've returned NULL\n");
    printf("getting first (deleted) entry:\n");
    if (get_routing_entry(&address) == NULL)
        printf("\tSuccess: address not in routing table, add_routing_entry() returned NULL\n");
    else
        printf("\tSomething went wrong, get_routing_entry() should've returned NULL\n");
}