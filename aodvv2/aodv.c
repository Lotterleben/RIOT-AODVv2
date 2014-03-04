#include "aodv.h"
#include "include/aodvv2.h"

#define ENABLE_DEBUG (1)
#include "debug.h"

#define UDP_BUFFER_SIZE     (128) // TODO öhm.
#define RCV_MSG_Q_SIZE      (64)

static void _init_addresses(void);
static void _init_sock_snd(void);
static void _aodv_receiver_thread(void);
static void _aodv_sender_thread(void);
static void _deep_free_msg_container(struct msg_container* msg_container);
static void _write_packet(struct rfc5444_writer *wr __attribute__ ((unused)),
        struct rfc5444_writer_target *iface __attribute__((unused)),
        void *buffer, size_t length);

char addr_str[IPV6_MAX_ADDR_STR_LEN];
char aodv_rcv_stack_buf[KERNEL_CONF_STACKSIZE_MAIN];
char aodv_snd_stack_buf[KERNEL_CONF_STACKSIZE_MAIN];

static int _metric_type;
static int sender_thread;
static int _sock_snd;
static struct autobuf _hexbuf;
static sockaddr6_t sa_wp;
static ipv6_addr_t _v6_addr_local, _v6_addr_mcast;
static struct netaddr na_local; // the same as _v6_addr_local, but to save us constant calls to ipv6_addr_t_to_netaddr()...
static struct writer_target* wt;
static struct netaddr_str nbuf;

void aodv_init(void)
{
    DEBUG("[aodvv2] %s()\n", __func__);

    aodv_set_metric_type(AODVV2_DEFAULT_METRIC_TYPE);
    _init_addresses();
    _init_sock_snd();

    /* init ALL the things! \o, */
    seqnum_init();
    routingtable_init();
    clienttable_init();

    /* every node is its own client. */
    clienttable_add_client(&na_local);
    rreqtable_init(); 

    /* init reader and writer */
    reader_init();
    writer_init(_write_packet);

    /* start listening & enable sending */
    thread_create(aodv_rcv_stack_buf, KERNEL_CONF_STACKSIZE_MAIN, PRIORITY_MAIN, CREATE_STACKTEST, _aodv_receiver_thread, "_aodv_receiver_thread");
    printf("[aodvv2] listening on port %d\n", HTONS(MANET_PORT));
    sender_thread = thread_create(aodv_snd_stack_buf, KERNEL_CONF_STACKSIZE_MAIN, PRIORITY_MAIN, CREATE_STACKTEST, _aodv_sender_thread, "_aodv_sender_thread");

    /* register aodv for routing */
    ipv6_iface_set_routing_provider(aodv_get_next_hop);

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

/**
 * Dispatch a rreq.
 */
void aodv_send_rreq(struct aodvv2_packet_data* packet_data)
{
    struct aodvv2_packet_data* pd = malloc(sizeof(struct aodvv2_packet_data));
    memcpy(pd, packet_data, sizeof(struct aodvv2_packet_data));

    struct rreq_rrep_data* rd = malloc(sizeof(struct rreq_rrep_data));
    *rd = (struct rreq_rrep_data) {
        .next_hop = &na_mcast,
        .packet_data = pd,
    };

    struct msg_container* mc = malloc(sizeof(struct msg_container));
    *mc = (struct msg_container) {
        .type = RFC5444_MSGTYPE_RREQ,
        .data = rd
    };

    msg_t msg;
    msg.content.ptr = mc;

    msg_send(&msg, sender_thread, false);
}

/**
 * Dispatch a rrep.
 */
void aodv_send_rrep(struct aodvv2_packet_data* packet_data, struct netaddr* next_hop)
{
    struct aodvv2_packet_data* pd = malloc(sizeof(struct aodvv2_packet_data));
    memcpy(pd, packet_data, sizeof(struct aodvv2_packet_data));

    struct netaddr* nh = malloc(sizeof(struct netaddr));
    memcpy(nh, next_hop, sizeof(struct netaddr));

    struct rreq_rrep_data* rd = malloc(sizeof(struct rreq_rrep_data));
    *rd = (struct rreq_rrep_data) {
        .next_hop = nh,
        .packet_data = pd,
    };

    struct msg_container* mc = malloc(sizeof(struct msg_container));
    *mc = (struct msg_container) {
        .type = RFC5444_MSGTYPE_RREQ,
        .data = rd
    };

    msg_t msg;
    msg.content.ptr = mc;

    msg_send(&msg, sender_thread, false);
}

/**
 * Dispatch a rerr.
 */
void aodv_send_rerr(struct unreachable_node unreachable_nodes[], int len, int hoplimit, struct netaddr* next_hop)
{
    struct rerr_data* rerrd = malloc(sizeof(struct rerr_data));
    *rerrd = (struct rerr_data) {
        .unreachable_nodes = unreachable_nodes,
        .len = len,
        .hoplimit = AODVV2_MAX_HOPCOUNT,
        .next_hop = next_hop
    };

    struct msg_container* mc2 = malloc(sizeof(struct msg_container));
    *mc2 = (struct msg_container) {
        .type = RFC5444_MSGTYPE_RERR,
        .data = rerrd
    };

    msg_t msg2;
    msg2.content.ptr = mc2;

    msg_send(&msg2, sender_thread, false);
}


/* 
 * init the multicast address all RREQs are sent to 
 * and the local address (source address) of this node
 */
static void _init_addresses(void)
{
    /* init multicast address: set to to a link-local all nodes multicast address */
    ipv6_addr_set_all_nodes_addr(&_v6_addr_mcast);
    DEBUG("[aodvv2] my multicast address is: %s\n", ipv6_addr_to_str(addr_str, &_v6_addr_mcast));

    /* get best IP for sending */
    ipv6_iface_get_best_src_addr(&_v6_addr_local, &_v6_addr_mcast);
    DEBUG("[aodvv2] my src address is:       %s\n", ipv6_addr_to_str(addr_str, &_v6_addr_local));

    /* store src & multicast address as netaddr as well for easy interaction 
    with oonf based stuff */
    ipv6_addr_t_to_netaddr(&_v6_addr_local, &na_local);
    ipv6_addr_t_to_netaddr(&_v6_addr_mcast, &na_mcast);

    /* init sockaddr that write_packet will use to send data */
    sa_wp.sin6_family = AF_INET6;
    sa_wp.sin6_port = HTONS(MANET_PORT);
}

/* Init everything needed for socket communication */
static void _init_sock_snd(void)
{
    _sock_snd = destiny_socket(PF_INET6, SOCK_DGRAM, IPPROTO_UDP);

    if(-1 == _sock_snd) {
        DEBUG("[aodvv2] Error Creating Socket!");
        return;
    }
}

/* Build and dispatch RREQs, RREPs and RERRs */
static void _aodv_sender_thread(void)
{
    msg_t msgq[1];
    msg_init_queue(msgq, sizeof msgq);
    DEBUG("[aodvv2] _aodv_sender_thread initialized.\n");

    while (true) {
        msg_t msg;
        msg_receive(&msg);
        struct msg_container* mc = (struct msg_container*) msg.content.ptr;

        if (mc->type == RFC5444_MSGTYPE_RREQ) {
            struct rreq_rrep_data* rreq_data = (struct rreq_rrep_data*) mc->data;
            writer_send_rreq(rreq_data->packet_data, rreq_data->next_hop);
        } else if (mc->type == RFC5444_MSGTYPE_RREP) {
            struct rreq_rrep_data* rrep_data = (struct rreq_rrep_data*) mc->data;
            writer_send_rrep(rrep_data->packet_data, rrep_data->next_hop); 
        } else if (mc->type == RFC5444_MSGTYPE_RERR) { 
            struct rerr_data* rerr_data = (struct rerr_data*) mc->data;
            writer_send_rerr(rerr_data->unreachable_nodes, rerr_data->len, rerr_data->hoplimit, rerr_data->next_hop);
        }
        else {
            DEBUG("ERROR: Couldn't identify Message");
        }
        _deep_free_msg_container(mc);
    }
}

/* receive RREQs and RREPs and handle them */
static void _aodv_receiver_thread(void)
{
    DEBUG("[aodvv2] %s()\n", __func__);
    uint32_t fromlen;
    int32_t rcv_size;
    char buf_rcv[UDP_BUFFER_SIZE];
    char addr_str_rec[IPV6_MAX_ADDR_STR_LEN];
    msg_t msg_q[RCV_MSG_Q_SIZE];
    
    msg_init_queue(msg_q, RCV_MSG_Q_SIZE);

    sockaddr6_t sa_rcv = { .sin6_family = AF_INET6,
                           .sin6_port = HTONS(MANET_PORT) };

    int sock_rcv = destiny_socket(PF_INET6, SOCK_DGRAM, IPPROTO_UDP);
    
    if (-1 == destiny_socket_bind(sock_rcv, &sa_rcv, sizeof(sa_rcv))) {
        DEBUG("Error: bind to recieve socket failed!\n");
        destiny_socket_close(sock_rcv);
    }

    DEBUG("[aodvv2] ready to receive data\n");
    for(;;) {
        rcv_size = destiny_socket_recvfrom(sock_rcv, (void *)buf_rcv, UDP_BUFFER_SIZE, 0, 
                                          &sa_rcv, &fromlen);

        if(rcv_size < 0) {
            DEBUG("[aodvv2] ERROR receiving data!\n");
        }
        DEBUG("[aodvv2] UDP packet received from %s\n", ipv6_addr_to_str(addr_str_rec, &sa_rcv.sin6_addr));
        
        struct netaddr _sender;
        ipv6_addr_t_to_netaddr(&sa_rcv.sin6_addr, &_sender);
        reader_handle_packet((void*) buf_rcv, rcv_size, &_sender);
    }

    destiny_socket_close(sock_rcv);    
}

/**
 * When set as ipv6_iface_routing_provider, this function will be called by 
 * RIOT's ipv6_sendto() to determine the next hop it should send a packet to dest to.
 * @param dest 
 * @return ipv6_addr_t* of the next hop towards dest if there is any, NULL if there is no next hop (yet)
 */
static ipv6_addr_t* aodv_get_next_hop(ipv6_addr_t* dest)
{
    DEBUG("[aodvv2] getting next hop for %s\n", ipv6_addr_to_str(addr_str, dest));

    struct netaddr _tmp_dest;
    ipv6_addr_t_to_netaddr(dest, &_tmp_dest);
    timex_t now;
    struct unreachable_node unreachable_nodes[AODVV2_MAX_UNREACHABLE_NODES];
    int len;

    struct aodvv2_routing_entry_t* rt_entry = routingtable_get_entry(&_tmp_dest, _metric_type);
    if (rt_entry) {
        /* 
           TODO use ndp_neighbor_get_ll_address() as soon as it's in the master.
           note: delete check for active/stale/delayed entries, get_ll_address
           does that for us then
        */
        ndp_neighbor_cache_t* ndp_nc_entry = ndp_neighbor_cache_search(dest);

        // Case 1: Undeliverable Packet        
        if (rt_entry->state == ROUTE_STATE_BROKEN ||
            rt_entry->state == ROUTE_STATE_EXPIRED ) {
            DEBUG("\tRouting table entry found, but invalid. Sending RERR.\n");
            unreachable_nodes[0].addr = _tmp_dest;
            unreachable_nodes[0].seqnum = rt_entry->seqnum;
            aodv_send_rerr(unreachable_nodes, 1, AODVV2_MAX_HOPCOUNT, &na_mcast);
            return NULL;
        }

        // Case 2: Broken Link
        if ((!ndp_nc_entry ||
            ndp_nc_entry->state != NDP_NCE_STATUS_REACHABLE ||
            ndp_nc_entry->state != NDP_NCE_STATUS_STALE ||
            ndp_nc_entry->state != NDP_NCE_STATUS_DELAY) &&
            (rt_entry->state != ROUTE_STATE_BROKEN)) {
            // mark all routes (active, idle, expired) that use next_hop as broken
            // and add all *Active* routes to the list of unreachable nodes        
            routingtable_break_and_get_all_hopping_over(&_tmp_dest, unreachable_nodes, &len);

            aodv_send_rerr(unreachable_nodes, len, AODVV2_MAX_HOPCOUNT, &na_mcast);
            return NULL;
        } 

        DEBUG("\t found dest in routing table: %s\n", netaddr_to_string(&nbuf, &rt_entry->nextHopAddr));

        vtimer_now(&now);
        rt_entry->lastUsed = now;
        if (rt_entry->state == ROUTE_STATE_IDLE)
            rt_entry->state = ROUTE_STATE_ACTIVE;

        return &rt_entry->nextHopAddr;
    } 

    struct aodvv2_packet_data rreq_data = (struct aodvv2_packet_data) {
        .hoplimit = AODVV2_MAX_HOPCOUNT,
        .metricType = _metric_type,
        .origNode = (struct node_data) {
            .addr = na_local,
            .metric = 0,
            .seqnum = seqnum_get(),
        },
        .targNode = (struct node_data) { 
            .addr = _tmp_dest,
        }
    };

    /* no route found => start route discovery */
    aodv_send_rreq(&rreq_data);

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
    
    /* fetch the address the packet is supposed to be sent to (i.e. to a 
       specific node or the multicast address) from the writer_target struct
       iface* is stored in. This is a bit hacky, but it does the trick. */
    wt = container_of(iface, struct writer_target, interface);
    netaddr_to_ipv6_addr_t(&wt->target_addr, &sa_wp.sin6_addr);

    /* When originating a RREQ, add it to our RREQ table/update its predecessor */
    if (wt->type == RFC5444_MSGTYPE_RREQ &&
        netaddr_cmp(&wt->packet_data.origNode.addr, &na_local) == 0) {
        DEBUG("[aodvv2] originating RREQ; updating RREQ table...\n");
        rreqtable_is_redundant(&wt->packet_data);
    }

    int bytes_sent = destiny_socket_sendto(_sock_snd, buffer, length, 
                                            0, &sa_wp, sizeof sa_wp);

    DEBUG("[aodvv2] %d bytes sent.\n", bytes_sent);
}

/* free the matryoshka doll of cobbled-together structs that the sender_thread receives */
static void _deep_free_msg_container(struct msg_container* mc)
{
    int type = mc->type;
    if ((type == RFC5444_MSGTYPE_RREQ) || (type == RFC5444_MSGTYPE_RREP)) {
        struct rreq_rrep_data* rreq_rrep_data = (struct rreq_rrep_data*) mc->data;
        free(rreq_rrep_data->packet_data);
        if (netaddr_cmp(rreq_rrep_data->next_hop, &na_mcast) != 0)
            free(rreq_rrep_data->next_hop);
    } else if (type == RFC5444_MSGTYPE_RERR) {
        // TODO: unreachable_nodes[] freen?
        struct rerr_data* rerr_data = (struct rerr_data*) mc->data;
        if (netaddr_cmp(rerr_data->next_hop, &na_mcast) != 0)
            free(rerr_data->next_hop);
    }
    free(mc->data);
    free(mc);
}
