/* Some aodvv2 utilities */

#include "utils.h"

static struct aodvv2_client_addresses client_table[AODVV2_MAX_CLIENTS];

void init_clienttable(void)
{   
    for (uint8_t i = 0; i < AODVV2_MAX_CLIENTS; i++) {
        memset(&client_table[i], 0, sizeof(client_table[i]));
    }
    printf("[aodvv2] client table initialized.\n");
}


/*
 * Add client to the list of clients that the router currently serves. 
 * Since the current version doesn't offer support for Client Networks,
 * the prefixlen is currently ignored.
 */
void add_client(struct netaddr* addr, uint8_t prefixlen)
{
    if(!is_client(addr, prefixlen)){
        /*find free spot in client table and place client address there */
        for (uint8_t i = 0; i < AODVV2_MAX_CLIENTS; i++){
            if (client_table[i].address._type == AF_UNSPEC
                && client_table[i].prefixlen == 0) {
                client_table[i].address = *addr;
                client_table[i].prefixlen = prefixlen; 
                return;
            }
        }
    }
}

/*
 * Find out if a client is in the list of clients that the router currently serves. 
 * Since the current version doesn't offer support for Client Networks,
 * the prefixlen is currently ignored.
 */
bool is_client(struct netaddr* addr, uint8_t prefixlen)
{
    for (uint8_t i = 0; i < AODVV2_MAX_CLIENTS; i++) {
        if (ipv6_addr_is_equal(&client_table[i].address, addr))
            return true;
    }
    return false;
}

/*
 * Selete a client from the list of clients that the router currently serves. 
 * Since the current version doesn't offer support for Client Networks,
 * the prefixlen is currently ignored.
 */
void delete_client(struct netaddr* addr, uint8_t prefixlen)
{
    if (!is_client(addr, prefixlen))
        return;
    
    for (uint8_t i = 0; i < AODVV2_MAX_CLIENTS; i++) {
        if (ipv6_addr_is_equal(&client_table[i].address, addr)) {
            memset(&client_table[i], 0, sizeof(client_table[i]));
            return;
        }
    }
}