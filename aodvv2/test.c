/* No real/proper tests, just some methods to check rotine things */

#import "routing.h"
#include "common/netaddr.h"
#include "sender.h"

void test_rt(void)
{
    uint8_t success;
    init_routingtable();
    ipv6_addr_t address, next_hop;

    /* init data */
    ipv6_addr_set_loopback_addr(&address);
    ipv6_addr_set_all_nodes_addr(&next_hop);

    /* start testing */
    print_rt();
    printf("Adding first entry...\n");
    add_routing_entry(&address, 1, 2, &next_hop, 0, 13, 0, ROUTE_STATE_IDLE);
    print_rt();
    printf("Adding second entry...\n");
    add_routing_entry(&next_hop, 1, 2, &next_hop, 1, 1, 1, ROUTE_STATE_ACTIVE);
    print_rt();
    printf("Deleting first entry...\n");
    delete_routing_entry(&address);
    print_rt();
}
