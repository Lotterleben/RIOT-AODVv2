#include "aodv.h"

#define ENABLE_DEBUG (1)
#include "debug.h"

static char sender_thread_stack[KERNEL_CONF_STACKSIZE_MAIN];
static char receiver_thread_stack[KERNEL_CONF_STACKSIZE_MAIN];
static int sock;
static sockaddr6_t sa;

void aodv_init(void)
{
    DEBUG("[aodvv2] %s()\n", __func__);
    /* init ALL the things! \o, */
    init_seqNum();
    init_routingtable();
    init_clienttable();
    init_rreqtable();

    reader_init();
    //writer_init(write_packet);

    /* start sending & receiving */

    /* register aodv for routing */
    /* aodv.c:50:37: warning: incompatible pointer types passing 'struct netaddr *(struct netaddr *, uint8_t)' to parameter of type 'ipv6_addr_t *(*)(ipv6_addr_t *)' [-Wincompatible-pointer-types]
        why the fuck.*/
    ipv6_iface_set_routing_provider(aodv_get_next_hop);
}

void aodv_send(void)
{
    DEBUG("[aodvv2] %s()\n", __func__);

    int sock;
    sockaddr6_t sa;
    ipv6_addr_t ipaddr;
    int bytes_sent;
    int address;
    char addr_str[IPV6_MAX_ADDR_STR_LEN];

    sock = destiny_socket(PF_INET6, SOCK_DGRAM, IPPROTO_UDP);

    if(-1 == sock) {
        printf("Error Creating Socket!");
        return;
    }

    memset(&sa, 0, sizeof sa);

    ipv6_addr_init(&ipaddr, 0xabcd, 0x0, 0x0, 0x0, 0x3612, 0x00ff, 0xfe00, (uint16_t)address);

    sa.sin6_family = AF_INET;
    memcpy(&sa.sin6_addr, &ipaddr, 16);
    sa.sin6_port = HTONS(MANET_PORT);

    bytes_sent = destiny_socket_sendto(sock, (char *)"hi",
            strlen("hi") + 1, 0, &sa,
            sizeof sa);

    if(bytes_sent < 0) {
        printf("Error sending packet!\n");
    }
    else {
        printf("Successful deliverd %i bytes over UDP to %s to 6LoWPAN\n", bytes_sent, ipv6_addr_to_str(addr_str, &ipaddr));
    }

    destiny_socket_close(sock);
}

static ipv6_addr_t* aodv_get_next_hop(ipv6_addr_t* dest)
{
    // TODO
    return NULL;
}












