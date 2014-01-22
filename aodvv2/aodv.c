#include "aodv.h"

#define ENABLE_DEBUG (1)
#include "debug.h"

char addr_str[IPV6_MAX_ADDR_STR_LEN];
char addr_str2[IPV6_MAX_ADDR_STR_LEN];

static void _init_addresses(void);
static void _init_sock_snd(void);
static void _write_packet(struct rfc5444_writer *wr __attribute__ ((unused)),
        struct rfc5444_writer_target *iface __attribute__((unused)),
        void *buffer, size_t length);
static void ipv6_addr_t_to_netaddr(ipv6_addr_t* src, struct netaddr* dst);

static int _metric_type;
static int sock_snd;
static struct autobuf _hexbuf;
static sockaddr6_t sa_mcast;
static ipv6_addr_t na_local;

void aodv_init(void)
{
    DEBUG("[aodvv2] %s()\n", __func__);

    aodv_set_metric_type(AODVV2_DEFAULT_METRIC_TYPE);
    _init_addresses();
    _init_sock_snd();

    /* init ALL the things! \o, */
    init_seqNum();
    init_routingtable();
    init_clienttable();
    /* every node is its own client. */
    struct netaddr _tmp;
    ipv6_addr_t_to_netaddr(&na_local, &_tmp);
    add_client(&_tmp, 128);
    init_rreqtable(); 

    /* init reader and writer */
    reader_init();
    writer_init(_write_packet);

    /* start sending & receiving */
    // TODO
    
    /* register aodv for routing */
    ipv6_iface_set_routing_provider(aodv_get_next_hop);
    ipv6_addr_t test_addr;
    ipv6_addr_init(&test_addr, 0xABCD, 0xEF12, 0, 0, 0x1034, 0x00FF, 0xFE00, 23);
    aodv_get_next_hop(&test_addr);
}

/* 
 * Change or set the metric type.
 * If metric_type does not match any known metric types, no changes will be made.
 */
void aodv_set_metric_type(int metric_type)
{
    if (metric_type != AODVV2_DEFAULT_METRIC_TYPE)
        return;
    _metric_type = metric_type;
}

/* 
 * init the multicast address all RREQs are sent to 
 * and the local address (source address) of this node
 */
static void _init_addresses(void)
{
    sa_mcast.sin6_family = AF_INET6;
    sa_mcast.sin6_port = HTONS(MANET_PORT);
    /* set to to a link-local all nodes multicast address */
    ipv6_addr_set_all_nodes_addr(&sa_mcast.sin6_addr);
    DEBUG("[aodvv2] my multicast address is: %s\n", ipv6_addr_to_str(&addr_str, &sa_mcast.sin6_addr));

    // TODO: fix this.
    ipv6_iface_get_best_src_addr(&na_local, &sa_mcast.sin6_addr);
    DEBUG("[aodvv2] my src address is:       %s\n", ipv6_addr_to_str(&addr_str2, &na_local));
}

/* Init everything needed for socket communication */
static void _init_sock_snd(void)
{
    sock_snd = destiny_socket(PF_INET6, SOCK_DGRAM, IPPROTO_UDP);

    if(-1 == sock_snd) {
        printf("[aodvv2] Error Creating Socket!");
        return;
    }
}

static ipv6_addr_t* aodv_get_next_hop(ipv6_addr_t* dest)
{
    ipv6_addr_t* next_hop = (ipv6_addr_t*) routingtable_get_next_hop(dest, _metric_type);
    if (next_hop)
        return next_hop;
    /* no route found => start route discovery */
    struct netaddr _tmp;
    ipv6_addr_t_to_netaddr(&na_local, &_tmp);
    writer_send_rreq(&_tmp, (struct netaddr*) dest);

    return NULL;
}

/**
 * Handle the output of the RFC5444 packet creation process
 * @param wr
 * @param iface
 * @param buffer
 * @param length
 */
static void _write_packet(struct rfc5444_writer *wr __attribute__ ((unused)),
        struct rfc5444_writer_target *iface __attribute__((unused)),
        void *buffer, size_t length)
{
    DEBUG("[aodvv2] %s()\n", __func__);

    /* generate hexdump and human readable representation of packet
        and print to console */
    abuf_hexdump(&_hexbuf, "\t", buffer, length);
    rfc5444_print_direct(&_hexbuf, buffer, length);
    DEBUG("%s", abuf_getptr(&_hexbuf));
    abuf_clear(&_hexbuf);
    
    int bytes_sent = destiny_socket_sendto(sock_snd, buffer, length, 0, &sa_mcast, sizeof sa_mcast);

    DEBUG("[aodvv2] %d bytes sent.\n", bytes_sent);
}

// TODO: so bauen dass es den ipv6_addr_t* direkt zurückgibt
static void ipv6_addr_t_to_netaddr(ipv6_addr_t* src, struct netaddr* dst)
{
    dst->_type = AF_INET6;
    dst->_prefix_len = AODVV2_RIOT_PREFIXLEN;
    memcpy(dst->_addr, src , sizeof dst->_addr);
}







