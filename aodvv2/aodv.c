#include <sixlowpan/ip.h>

#include "include/aodvv2.h"
#include "seqnum.h"
#include "routing.h" 
#include "utils.h"
#include "reader.h"
#include "writer.h"

static void write_packet(struct rfc5444_writer *wr __attribute__ ((unused)),
    struct rfc5444_writer_target *iface __attribute__((unused)),
    void *buffer, size_t length);
static ipv6_addr_t* get_next_hop(ipv6_addr_t* dest);

void aodv_init(void)
{
    // TODO: init transceiver etc
    
    /* init ALL the things! \o, */
    init_seqNum();
    init_routingtable();
    init_clienttable();
    init_rreqtable();

    reader_init();
    writer_init(write_packet);

    /* register aodv for routing */
    ipv6_iface_set_routing_provider(get_next_hop);
}


static void write_packet(struct rfc5444_writer *wr __attribute__ ((unused)),
    struct rfc5444_writer_target *iface __attribute__((unused)),
    void *buffer, size_t length)
{
    //TODO
}

static ipv6_addr_t* get_next_hop(ipv6_addr_t* dest)
{
    // TODO
    return NULL;
}

static void aodv_sender_thread(void)
{
    // TODO
}

static void aodv_reciever_thread(void)
{
    // TODO
}

static void aodv_route_discovery_thread(void)
{
    // TODO
}