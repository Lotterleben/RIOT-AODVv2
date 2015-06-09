#include "virtualnetwork.h"

int virtualnetwork_sendto(int s, const void *buf, uint32_t len, int flags,
                              sockaddr6_t *to, socklen_t tolen)
{
    (void)s;
    (void)buf;
    (void)len;
    (void)flags;
    (void)to;
    (void)tolen;

    printf("%s: IMPLEMENT ME!\n", __func__);
    return 1;
}

void virtualnetwork_set_routing_provider(ipv6_addr_t *(*next_hop)(ipv6_addr_t *dest))
{
    (void)next_hop;
    printf("%s: IMPLEMENT ME!\n", __func__);
}
