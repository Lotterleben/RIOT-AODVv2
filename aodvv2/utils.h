/* Some aodvv2 utilities */

#include "common/netaddr.h"
#include "ipv6.h"

#define AODVV2_MAX_CLIENTS 16 // shabby, I know

struct aodvv2_client_addresses {
    struct netaddr address;
    uint8_t prefixlen; //should be long enough, no?
};

void init_clienttable(void);
void add_client(struct netaddr* addr, uint8_t prefixlen);
bool is_client(struct netaddr* addr, uint8_t prefixlen);
void delete_client(struct netaddr* addr, uint8_t prefixlen);