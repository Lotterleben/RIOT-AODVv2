/* No real/proper tests, just some methods to check rotine things */

#import "routing.h"
#include "common/netaddr.h"

void test_rt(void)
{
    uint8_t success;
    init_routingtable();
    ipv6_addr_t address, next_hop;
    char addr_str[IPV6_MAX_ADDR_STR_LEN];

    /* init data */
    ipv6_addr_set_loopback_addr(&address);
    ipv6_addr_set_all_nodes_addr(&next_hop);

    /* start testing */
    print_rt();
    printf("Adding first entry: %s ...\n", ipv6_addr_to_str(addr_str, &address));
    add_routing_entry(&address, 1, 2, &next_hop, 0, 13, 0, ROUTE_STATE_IDLE);
    print_rt();
    printf("Adding second entry: %s ...\n", ipv6_addr_to_str(addr_str, &next_hop));
    add_routing_entry(&next_hop, 1, 2, &next_hop, 1, 1, 1, ROUTE_STATE_ACTIVE);
    print_rt();
    printf("Deleting first entry: %s ...\n", ipv6_addr_to_str(addr_str, & address));
    delete_routing_entry(&address);
    print_rt();
    printf("getting next hop of second entry:\n");
    printf("\t%s\n",ipv6_addr_to_str(addr_str, get_next_hop(&next_hop)));
}