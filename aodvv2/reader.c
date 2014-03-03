#ifdef RIOT
#include "net_help.h"
#endif

#include "reader.h"

#define ENABLE_DEBUG (1)
#include "debug.h"

static enum rfc5444_result _cb_rreq_blocktlv_addresstlvs_okay(
    struct rfc5444_reader_tlvblock_context *cont);
static enum rfc5444_result _cb_rreq_blocktlv_messagetlvs_okay(
    struct rfc5444_reader_tlvblock_context *cont);
static enum rfc5444_result _cb_rreq_end_callback(
    struct rfc5444_reader_tlvblock_context *cont, bool dropped);

static enum rfc5444_result _cb_rrep_blocktlv_addresstlvs_okay(
    struct rfc5444_reader_tlvblock_context *cont);
static enum rfc5444_result _cb_rrep_blocktlv_messagetlvs_okay(
    struct rfc5444_reader_tlvblock_context *cont);
static enum rfc5444_result _cb_rrep_end_callback(
    struct rfc5444_reader_tlvblock_context *cont, bool dropped);

static enum rfc5444_result _cb_rerr_blocktlv_addresstlvs_okay(
    struct rfc5444_reader_tlvblock_context *cont);
static enum rfc5444_result _cb_rerr_blocktlv_messagetlvs_okay(
    struct rfc5444_reader_tlvblock_context *cont);
static enum rfc5444_result _cb_rerr_end_callback(
    struct rfc5444_reader_tlvblock_context *cont, bool dropped);

/* helper functions */
static bool _offers_improvement(struct aodvv2_routing_entry_t* rt_entry, struct node_data* node_data);
static uint8_t _get_link_cost(uint8_t metricType, struct aodvv2_packet_data* data);
static uint8_t _get_max_metric(uint8_t metricType);
static uint8_t _get_updated_metric(uint8_t metricType, uint8_t metric);
static void _fill_routing_entry_t_rreq(struct aodvv2_packet_data* packet_data, struct aodvv2_routing_entry_t* routing_entry, uint8_t link_cost);
static void _fill_routing_entry_t_rrep(struct aodvv2_packet_data* packet_data, struct aodvv2_routing_entry_t* rt_entry, uint8_t link_cost);

/* This is where we store data gathered from packets */
static struct aodvv2_packet_data packet_data;
static struct unreachable_node unreachable_nodes[AODVV2_MAX_UNREACHABLE_NODES];
static int num_unreachable_nodes;

static struct rfc5444_reader reader;
static timex_t validity_t;

/*
 * Message consumer, will be called once for every message of
 * type RFC5444_MSGTYPE_RREQ that contains all the mandatory message TLVs
 */
static struct rfc5444_reader_tlvblock_consumer _rreq_consumer = {
    .msg_id = RFC5444_MSGTYPE_RREQ,
    .block_callback = _cb_rreq_blocktlv_messagetlvs_okay,
    .end_callback = _cb_rreq_end_callback,
};

/*
 * Address consumer. Will be called once for every address in a message of
 * type RFC5444_MSGTYPE_RREQ.
 */
static struct rfc5444_reader_tlvblock_consumer _rreq_address_consumer = {
    .msg_id = RFC5444_MSGTYPE_RREQ,
    .addrblock_consumer = true,
    .block_callback = _cb_rreq_blocktlv_addresstlvs_okay,
};
 
/*
 * Message consumer, will be called once for every message of
 * type RFC5444_MSGTYPE_RREP that contains all the mandatory message TLVs
 */
static struct rfc5444_reader_tlvblock_consumer _rrep_consumer = {
    .msg_id = RFC5444_MSGTYPE_RREP,
    .block_callback = _cb_rrep_blocktlv_messagetlvs_okay,
    .end_callback = _cb_rrep_end_callback,
};

/*
 * Address consumer. Will be called once for every address in a message of
 * type RFC5444_MSGTYPE_RREP.
 */
static struct rfc5444_reader_tlvblock_consumer _rrep_address_consumer = {
    .msg_id = RFC5444_MSGTYPE_RREP,
    .addrblock_consumer = true,
    .block_callback = _cb_rrep_blocktlv_addresstlvs_okay,
};

/*
 * Message consumer, will be called once for every message of
 * type RFC5444_MSGTYPE_RERR that contains all the mandatory message TLVs
 */
static struct rfc5444_reader_tlvblock_consumer _rerr_consumer = {
    .msg_id = RFC5444_MSGTYPE_RERR,
    .block_callback = _cb_rerr_blocktlv_messagetlvs_okay,
    .end_callback = _cb_rerr_end_callback,
};

/*
 * Address consumer. Will be called once for every address in a message of
 * type RFC5444_MSGTYPE_RERR.
 */
static struct rfc5444_reader_tlvblock_consumer _rerr_address_consumer = {
    .msg_id = RFC5444_MSGTYPE_RERR,
    .addrblock_consumer = true,
    .block_callback = _cb_rerr_blocktlv_addresstlvs_okay,
};


/*
 * Address consumer entries definition
 * TLV types RFC5444_MSGTLV__SEQNUM and RFC5444_MSGTLV_METRIC
 */
static struct rfc5444_reader_tlvblock_consumer_entry _rreq_rrep_address_consumer_entries[] = {
    [RFC5444_MSGTLV_ORIGSEQNUM] = { .type = RFC5444_MSGTLV_ORIGSEQNUM},
    [RFC5444_MSGTLV_TARGSEQNUM] = { .type = RFC5444_MSGTLV_TARGSEQNUM},
    [RFC5444_MSGTLV_METRIC] = { .type = AODVV2_DEFAULT_METRIC_TYPE }
};

/*
 * Address consumer entries definition
 * TLV types RFC5444_MSGTLV__SEQNUM and RFC5444_MSGTLV_METRIC
 */
static struct rfc5444_reader_tlvblock_consumer_entry _rerr_address_consumer_entries[] = {
    [RFC5444_MSGTLV_UNREACHABLE_NODE_SEQNUM] = { .type = RFC5444_MSGTLV_UNREACHABLE_NODE_SEQNUM},
};

/**
 * This block callback is called for every address of a RREQ Message.
 *
 * @param cont
 * @return
 */
static enum rfc5444_result _cb_rreq_blocktlv_addresstlvs_okay(struct rfc5444_reader_tlvblock_context *cont)
{
    struct netaddr_str nbuf;
    struct rfc5444_reader_tlvblock_entry* tlv;
    bool is_origNode_addr = false;
    bool is_targNode_addr = false;

    DEBUG("[aodvv2] %s()\n", __func__);
    DEBUG("\tmessage type: %d\n", cont->type);
    DEBUG("\taddr: %s\n", netaddr_to_string(&nbuf, &cont->addr));

    /* handle OrigNode SeqNum TLV */
    tlv = _rreq_rrep_address_consumer_entries[RFC5444_MSGTLV_ORIGSEQNUM].tlv;
    if (tlv) {
        DEBUG("\ttlv RFC5444_MSGTLV_ORIGSEQNUM: %d\n", *tlv->single_value);
        is_origNode_addr = true;
        packet_data.origNode.addr = cont->addr;
        packet_data.origNode.seqnum = *tlv->single_value;
    }

    /* handle TargNode SeqNum TLV */
    tlv = _rreq_rrep_address_consumer_entries[RFC5444_MSGTLV_TARGSEQNUM].tlv;
    if (tlv) {
        DEBUG("\ttlv RFC5444_MSGTLV_TARGSEQNUM: %d\n", *tlv->single_value);
        is_targNode_addr = true;
        packet_data.targNode.addr = cont->addr;
        packet_data.targNode.seqnum = *tlv->single_value;
    }
    if (!tlv && !is_origNode_addr) {
        /* assume that tlv missing => targNode Address */
        is_targNode_addr = true;
        packet_data.targNode.addr = cont->addr;
    }

    if (!is_origNode_addr && !is_targNode_addr) {
        DEBUG("\tERROR: mandatory RFC5444_MSGTLV_ORIGSEQNUM TLV missing.\n");
        return RFC5444_DROP_PACKET;        
    }

    /* handle Metric TLV */
    tlv = _rreq_rrep_address_consumer_entries[RFC5444_MSGTLV_METRIC].tlv;
    if (!tlv && is_origNode_addr){
        DEBUG("\tERROR: Missing or unknown metric TLV.\n");
        return RFC5444_DROP_PACKET;
    }
    if (tlv) {
        if (!is_origNode_addr){
            DEBUG("\tERROR: Metric TLV belongs to wrong address.\n");
            return RFC5444_DROP_PACKET;
        }
        DEBUG("\ttlv RFC5444_MSGTLV_METRIC hopCt: %d, type: %d\n", *tlv->single_value, tlv->type);
        packet_data.metricType = tlv->type;
        packet_data.origNode.metric = *tlv->single_value;
    }
    return RFC5444_OKAY;
}

/**
 * This block callback is called for every address
 *
 * @param cont
 * @return
 */
static enum rfc5444_result _cb_rreq_blocktlv_messagetlvs_okay(struct rfc5444_reader_tlvblock_context *cont)
{
    DEBUG("[aodvv2] %s()\n", __func__);

    if (!cont->has_hoplimit) {
        DEBUG("\tERROR: missing hop limit\n");
        return RFC5444_DROP_PACKET;
    } 

    packet_data.hoplimit = cont->hoplimit;
    if (packet_data.hoplimit == 0) {
        DEBUG("\tERROR: Hoplimit is 0.\n");
        return RFC5444_DROP_PACKET;
    }
    DEBUG("[aodvv2] %s()\n\t i can has hop limit: %d\n",__func__ , cont->hoplimit);
    packet_data.hoplimit--;
    return RFC5444_OKAY;
}

/**
 * This callback is called every time the _rreq_consumer finishes reading a
 * packet.
 * @param cont
 * @param dropped indicates wether the packet has been dropped previously by
 *                another callback
 */
static enum rfc5444_result _cb_rreq_end_callback(
    struct rfc5444_reader_tlvblock_context *cont, bool dropped)
{
    struct aodvv2_routing_entry_t* rt_entry;
    timex_t now;
    uint8_t link_cost = _get_link_cost(packet_data.metricType, &packet_data);

    DEBUG("[aodvv2] %s() dropped: %d\n", __func__, dropped);

    /* Check if packet contains the required information */
    if (dropped) {
        DEBUG("\t Dropping packet.\n");
        return RFC5444_DROP_PACKET;
    } 
    if ((packet_data.origNode.addr._type == AF_UNSPEC) || !packet_data.origNode.seqnum){
        DEBUG("\tERROR: missing OrigNode Address or SeqNum. Dropping packet.\n");
        return RFC5444_DROP_PACKET;
    }
    if (packet_data.targNode.addr._type == AF_UNSPEC){
        DEBUG("\tERROR: missing TargNode Address. Dropping packet.\n");
        return RFC5444_DROP_PACKET; 
    }
    if (packet_data.hoplimit == 0){
        DEBUG("\tERROR: Hoplimit is 0. Dropping packet.\n");
        return RFC5444_DROP_PACKET;
    }
    if ((_get_max_metric(packet_data.metricType) - link_cost) <= packet_data.origNode.metric){
        DEBUG("\tMetric Limit reached. Dropping packet.\n");
        return RFC5444_DROP_PACKET;
    }

    /*
      The incoming RREQ MUST be checked against previously received
      information from the RREQ Table Section 7.6.  If the information
      in the incoming RteMsg is redundant, then then no further action
      is taken.
    */
    if (rreqtable_is_redundant(&packet_data)){
        DEBUG("\tPacket is redundant. Dropping Packet. %i\n", RFC5444_DROP_PACKET);
        return RFC5444_DROP_PACKET;
    }

    packet_data.origNode.metric = _get_updated_metric(packet_data.metricType, packet_data.origNode.metric);
    vtimer_now(&now);
    packet_data.timestamp = now;

    /* for every relevant
     * address (RteMsg.Addr) in the RteMsg, HandlingRtr searches its route
     * table to see if there is a route table entry with the same MetricType
     * of the RteMsg, matching RteMsg.Addr.
     */

    rt_entry = routingtable_get_entry(&packet_data.origNode.addr, packet_data.metricType);

    if (!rt_entry || (rt_entry->metricType != packet_data.metricType)){
        DEBUG("\tCreating new Routing Table entry...\n");

        struct aodvv2_routing_entry_t* tmp_rt_entry = (struct aodvv2_routing_entry_t*)malloc(sizeof(struct aodvv2_routing_entry_t));
        memset(tmp_rt_entry, 0, sizeof(*tmp_rt_entry)); // TODO: muss ich das überhaupt? wird ja eh gefüllt

        _fill_routing_entry_t_rreq(&packet_data, tmp_rt_entry, link_cost);
        routingtable_add_entry(tmp_rt_entry);
        free(tmp_rt_entry);
    } else {
        if (!_offers_improvement(rt_entry, &packet_data.origNode)){
            DEBUG("\tPacket offers no improvement over known route. Dropping Packet.\n");
            return RFC5444_DROP_PACKET; 
        }
        /* The incoming routing information is better than existing routing 
         * table information and SHOULD be used to improve the route table. */ 
        DEBUG("\tUpdating Routing Table entry...\n");
        _fill_routing_entry_t_rreq(&packet_data, rt_entry, link_cost);
    }

    /*
     * If TargNode is a client of the router receiving the RREQ, then the
     * router generates a RREP message as specified in Section 7.4, and
     * subsequently processing for the RREQ is complete.  Otherwise,
     * processing continues as follows.
     */
    if (clienttable_is_client(&packet_data.targNode.addr)){
        DEBUG("[aodvv2] TargNode is in client list, sending RREP\n");    
        aodv_send_rrep(&packet_data, &packet_data.sender);
    }

    else {
        DEBUG("[aodvv2] I am not TargNode, forwarding RREQ\n");
        aodv_send_rreq(&packet_data);
    }
    return RFC5444_OKAY;
}

/**
 * This block callback is called for every address of a RREP Message.
 *
 * @param cont
 * @return
 */
static enum rfc5444_result _cb_rrep_blocktlv_addresstlvs_okay(struct rfc5444_reader_tlvblock_context *cont)
{
    struct netaddr_str nbuf;
    struct rfc5444_reader_tlvblock_entry* tlv;
    bool is_targNode_addr = false;

    DEBUG("[aodvv2] %s()\n", __func__);
    DEBUG("\tmessage type: %d\n", cont->type);
    DEBUG("\taddr: %s\n", netaddr_to_string(&nbuf, &cont->addr));

    /* handle TargNode SeqNum TLV */
    tlv = _rreq_rrep_address_consumer_entries[RFC5444_MSGTLV_TARGSEQNUM].tlv;
    if (tlv) {
        DEBUG("\ttlv RFC5444_MSGTLV_SEQNUM: %d\n", *tlv->single_value);
        is_targNode_addr = true;
        packet_data.targNode.addr = cont->addr;
        packet_data.targNode.seqnum = *tlv->single_value;
    }

    /* handle OrigNode SeqNum TLV */
    tlv = _rreq_rrep_address_consumer_entries[RFC5444_MSGTLV_ORIGSEQNUM].tlv;
    if (tlv) {
        DEBUG("\ttlv RFC5444_MSGTLV_ORIGSEQNUM: %d\n", *tlv->single_value);
        is_targNode_addr = false;
        packet_data.origNode.addr = cont->addr;
        packet_data.origNode.seqnum = *tlv->single_value;
    } 
    if (!tlv && !is_targNode_addr) {
        DEBUG("\tERROR: mandatory SeqNum TLV missing.\n");
        return RFC5444_DROP_PACKET;
    }

    /* handle Metric TLV */
    tlv = _rreq_rrep_address_consumer_entries[RFC5444_MSGTLV_METRIC].tlv;
    if (!tlv && is_targNode_addr){
        DEBUG("\tERROR: Missing or unknown metric TLV.\n");
        return RFC5444_DROP_PACKET;
    }
    if (tlv) {
        if (!is_targNode_addr){
            DEBUG("\tERROR: metric TLV belongs to wrong address.\n");
            return RFC5444_DROP_PACKET;
        }
        DEBUG("\ttlv RFC5444_MSGTLV_METRIC hopCt: %d, type: %d\n", *tlv->single_value, tlv->type);
        packet_data.metricType = tlv->type;
        packet_data.origNode.metric = *tlv->single_value;
    }
    return RFC5444_OKAY;
}

/**
 * This block callback is called for every address
 *
 * @param cont
 * @return
 */
static enum rfc5444_result _cb_rrep_blocktlv_messagetlvs_okay(struct rfc5444_reader_tlvblock_context *cont)
{
    DEBUG("[aodvv2] %s()\n", __func__);

    if (!cont->has_hoplimit) {
        DEBUG("\tERROR: missing hop limit\n");
        return RFC5444_DROP_PACKET;
    } 

    packet_data.hoplimit = cont->hoplimit;
    if (packet_data.hoplimit == 0) {
        DEBUG("\tERROR: Hoplimit is 0.\n");
        return RFC5444_DROP_PACKET;
    }
    DEBUG("[aodvv2] %s()\n\t i can has hop limit: %d\n",__func__ , cont->hoplimit);
    packet_data.hoplimit--;
    return RFC5444_OKAY;
}

/**
 * This callback is called every time the _rreq_consumer finishes reading a
 * packet.
 * @param cont
 * @param dropped indicates wehther the packet has been dropped previously by
 *                another callback
 */
static enum rfc5444_result _cb_rrep_end_callback(
    struct rfc5444_reader_tlvblock_context *cont, bool dropped)
{
    struct aodvv2_routing_entry_t* rt_entry;
    timex_t now;
    uint8_t link_cost = _get_link_cost(packet_data.metricType, &packet_data);

    DEBUG("[aodvv2] %s() dropped: %d\n", __func__, dropped);

    /* Check if packet contains the required information */
    if (dropped) {
        DEBUG("\t Dropping packet.\n");
        return RFC5444_DROP_PACKET;
    } 
    if ((packet_data.origNode.addr._type == AF_UNSPEC) || !packet_data.origNode.seqnum) {
        DEBUG("\tERROR: missing OrigNode Address or SeqNum. Dropping packet.\n");
        return RFC5444_DROP_PACKET;
    }
    if ((packet_data.targNode.addr._type == AF_UNSPEC) || !packet_data.targNode.seqnum) {
        DEBUG("\tERROR: missing TargNode Address or SeqNum. Dropping packet.\n");
        return RFC5444_DROP_PACKET; 
    }
    if ((_get_max_metric(packet_data.metricType) - link_cost) <= packet_data.targNode.metric){
        DEBUG("\tMetric Limit reached. Dropping packet.\n");
        return RFC5444_DROP_PACKET;
    }

    packet_data.origNode.metric = _get_updated_metric(packet_data.metricType, packet_data.origNode.metric);
    vtimer_now(&now);
    packet_data.timestamp = now;

    /* for every relevant
     * address (RteMsg.Addr) in the RteMsg, HandlingRtr searches its route
     * table to see if there is a route table entry with the same MetricType
     * of the RteMsg, matching RteMsg.Addr.
     */

    rt_entry = routingtable_get_entry(&packet_data.targNode.addr, packet_data.metricType);

    if (!rt_entry || (rt_entry->metricType != packet_data.metricType)){
        DEBUG("\tCreating new Routing Table entry...\n");

        struct aodvv2_routing_entry_t* tmp_rt_entry = (struct aodvv2_routing_entry_t*)malloc(sizeof(struct aodvv2_routing_entry_t));
        memset(tmp_rt_entry, 0, sizeof(*tmp_rt_entry)); // TODO: muss ich das überhaupt? wird ja eh gefüllt

        _fill_routing_entry_t_rrep(&packet_data, tmp_rt_entry, link_cost);
        routingtable_add_entry(tmp_rt_entry);

        free(tmp_rt_entry);
    } else {
        if (!_offers_improvement(rt_entry, &packet_data.targNode)) {
            DEBUG("\tPacket offers no improvement over known route. Dropping Packet.\n");
            return RFC5444_DROP_PACKET; 
        }
        /* The incoming routing information is better than existing routing 
         * table information and SHOULD be used to improve the route table. */ 
        DEBUG("\tUpdating Routing Table entry...\n");
        _fill_routing_entry_t_rreq(&packet_data, rt_entry, link_cost);
    }
    
    /*
    If HandlingRtr is RREQ_Gen then the RREP satisfies RREQ_Gen's
    earlier RREQ, and RREP processing is completed.  Any packets
    buffered for OrigNode should be transmitted. */
    if (clienttable_is_client(&packet_data.origNode.addr)){
        DEBUG("\tThis is my RREP. We are done here, thanks!\n");
    }

    /* 
    If HandlingRtr is not RREQ_Gen then the outgoing RREP is sent to the
    Route.NextHopAddress for the RREP.AddrBlk[OrigNodeNdx]. */
    else {
        DEBUG("[aodvv2] Not my RREP, passing it on to the next hop\n");
        aodv_send_rrep(&packet_data, routingtable_get_next_hop(&packet_data.origNode.addr, packet_data.metricType));
    }
    return RFC5444_OKAY;
}

static enum rfc5444_result _cb_rerr_blocktlv_messagetlvs_okay(struct rfc5444_reader_tlvblock_context *cont)
{
    DEBUG("[aodvv2] %s()\n", __func__);

    if (!cont->has_hoplimit) {
        DEBUG("\tERROR: missing hop limit\n");
        return RFC5444_DROP_PACKET;
    } 

    packet_data.hoplimit = cont->hoplimit;
    if (packet_data.hoplimit == 0) {
        DEBUG("\tERROR: Hoplimit is 0.\n");
        return RFC5444_DROP_PACKET;
    }
    DEBUG("[aodvv2] %s()\n\t i can has hop limit: %d\n",__func__ , cont->hoplimit);
    packet_data.hoplimit--;

    /* prepare buffer for unreachable nodes */
    num_unreachable_nodes = 0;
    for (uint8_t i = 0; i < AODVV2_MAX_UNREACHABLE_NODES; i++) {
        memset(&unreachable_nodes[i], 0, sizeof(unreachable_nodes[i]));
    }
    return RFC5444_OKAY;
}

static enum rfc5444_result _cb_rerr_blocktlv_addresstlvs_okay(struct rfc5444_reader_tlvblock_context *cont)
{
    struct netaddr_str nbuf;
    struct aodvv2_routing_entry_t* unreachable_entry;
    struct rfc5444_reader_tlvblock_entry* tlv;

    DEBUG("[aodvv2] %s()\n", __func__);
    DEBUG("\tmessage type: %d\n", cont->type);
    DEBUG("\taddr: %s\n", netaddr_to_string(&nbuf, &cont->addr));  

    /* Out of buffer size for more unreachable nodes. We're screwed, basically. */
    if (num_unreachable_nodes == AODVV2_MAX_UNREACHABLE_NODES)
        return RFC5444_OKAY;

    // gather packet data TODO: will this fail if there's no seqnum?
    packet_data.origNode.addr = cont->addr;

    /* handle this unreachable node's SeqNum TLV */
    tlv = _rerr_address_consumer_entries[RFC5444_MSGTLV_UNREACHABLE_NODE_SEQNUM].tlv;
    if (tlv) {
        DEBUG("\ttlv RFC5444_MSGTLV_UNREACHABLE_NODE_SEQNUM: %d\n", *tlv->single_value);
        packet_data.origNode.seqnum = *tlv->single_value;
    }

    /* Check if there is an entry for unreachable node in our routing table */
    unreachable_entry = routingtable_get_entry(&packet_data.origNode.addr, packet_data.metricType);
    if (unreachable_entry) {
        /* check if route to unreachable node has to be marked as broken and RERR has to be forwarded*/
        if (netaddr_cmp(&unreachable_entry->nextHopAddr, &packet_data.sender ) == 0 
            && (!tlv || seqnum_cmp(unreachable_entry->seqnum, packet_data.origNode.seqnum))) {
            unreachable_entry->state = ROUTE_STATE_BROKEN; // TODO: debug because it will break routesfor a long time
            unreachable_nodes[num_unreachable_nodes].addr = packet_data.origNode.addr;
            unreachable_nodes[num_unreachable_nodes].seqnum = packet_data.origNode.seqnum;
            num_unreachable_nodes++;
        }
    }
    return RFC5444_OKAY;
}

static enum rfc5444_result _cb_rerr_end_callback(struct rfc5444_reader_tlvblock_context *cont, bool dropped)
{
    DEBUG("[aodvv2] %s() dropped: %d\n", __func__, dropped);
    
    if (dropped) {
        DEBUG("\tDropping packet.\n");
        return RFC5444_DROP_PACKET;
    } 

    if ( num_unreachable_nodes == 0){
        DEBUG("\tNo unreachable nodes from my routing table. Dropping Packet.\n");
        return RFC5444_DROP_PACKET;
    }
    /* gather all unreachable nodes and put them into a RERR */
    aodv_send_rerr(unreachable_nodes, num_unreachable_nodes, packet_data.hoplimit, &na_mcast);
    return RFC5444_OKAY;
}

void reader_init(void)
{
    DEBUG("[aodvv2] %s()\n", __func__);

    validity_t = timex_set(AODVV2_ACTIVE_INTERVAL + AODVV2_MAX_IDLETIME, 0); 

    /* initialize reader */
    rfc5444_reader_init(&reader);

    /* register message consumers. We have no message TLVs, so we can leave the
     rfc5444_reader_tlvblock_consumer_entry empty */
    rfc5444_reader_add_message_consumer(&reader, &_rreq_consumer,
        NULL, 0);
    rfc5444_reader_add_message_consumer(&reader, &_rrep_consumer,
        NULL, 0);
    rfc5444_reader_add_message_consumer(&reader, &_rerr_consumer,
        NULL, 0);

    /* register address consumer */
    rfc5444_reader_add_message_consumer(&reader, &_rreq_address_consumer,
        _rreq_rrep_address_consumer_entries, ARRAYSIZE(_rreq_rrep_address_consumer_entries));
    rfc5444_reader_add_message_consumer(&reader, &_rrep_address_consumer,
        _rreq_rrep_address_consumer_entries, ARRAYSIZE(_rreq_rrep_address_consumer_entries));
    rfc5444_reader_add_message_consumer(&reader, &_rerr_address_consumer,
        _rerr_address_consumer_entries, ARRAYSIZE(_rerr_address_consumer_entries));
}

void reader_cleanup(void)
{
    DEBUG("[aodvv2] %s()\n", __func__);
    rfc5444_reader_cleanup(&reader);
}

/**
 * 
 * @param sender Address of the node from which the packet was received
 */
int reader_handle_packet(void* buffer, size_t length, struct netaddr* sender)
{
    DEBUG("[aodvv2] %s()\n", __func__);
    memcpy(&packet_data.sender, sender, sizeof(*sender));
    return rfc5444_reader_handle_packet(&reader, buffer, length);
}

//============= HELPER FUNCTIONS ===============================================

/*
 * handle collected data as described in Section 6.1 
 */
static bool _offers_improvement(struct aodvv2_routing_entry_t* rt_entry, struct node_data* node_data)
{
    /* Check if new info is stale */    
    if (seqnum_cmp(node_data->seqnum, rt_entry->seqnum) == -1)
        return false;
    /* Check if new info is more costly */
    if ((node_data->metric >= rt_entry->metric) && !(rt_entry->state != ROUTE_STATE_BROKEN))
        return false;
    /* Check if new info repairs a broken route */
    if (!(rt_entry->state != ROUTE_STATE_BROKEN))
        return false;
    return true;
}

/*
 * Cost(L): Get Cost of a Link regarding the specified metric.
 * (currently only AODVV2_DEFAULT_METRIC_TYPE (HopCt) implemented)
 * returns cost if metric is known, NULL otherwise
 */
static uint8_t _get_link_cost(uint8_t metricType, struct aodvv2_packet_data* packet_data)
{
    if (metricType == AODVV2_DEFAULT_METRIC_TYPE)
        return 1;
    return 0;
}

/*
 * MAX_METRIC[MetricType]:
 * returns maximum value of the given metric if metric is known, NULL otherwise.
 */
static uint8_t _get_max_metric(uint8_t metricType)
{
    if (metricType == AODVV2_DEFAULT_METRIC_TYPE)
        return AODVV2_MAX_HOPCOUNT;
    return 0;
}

/*
 * Calculate a metric's new value according to the specified MetricType
 * (currently only implemented for AODVV2_DEFAULT_METRIC_TYPE (HopCt))
 */
static uint8_t _get_updated_metric(uint8_t metricType, uint8_t metric)
{
    if (metricType == AODVV2_DEFAULT_METRIC_TYPE)
        return metric++;
    return 0;
}

// TODO: use memcpy?!
/* Fills a routing table entry with the data of a RREQ */
static void _fill_routing_entry_t_rreq(struct aodvv2_packet_data* packet_data, struct aodvv2_routing_entry_t* rt_entry, uint8_t link_cost)
{
    rt_entry->addr = packet_data->origNode.addr;
    rt_entry->seqnum = packet_data->origNode.seqnum;
    rt_entry->nextHopAddr = packet_data->sender;
    rt_entry->lastUsed = packet_data->timestamp;
    rt_entry->expirationTime = timex_add(packet_data->timestamp, validity_t);
    rt_entry->metricType = packet_data->metricType;
    rt_entry->metric = packet_data->origNode.metric + link_cost;
    rt_entry->state = ROUTE_STATE_ACTIVE;
}

// TODO: use memcpy?!
/* Fills a routing table entry with the data of a RREQ */
static void _fill_routing_entry_t_rrep(struct aodvv2_packet_data* packet_data, struct aodvv2_routing_entry_t* rt_entry, uint8_t link_cost)
{
    rt_entry->addr = packet_data->targNode.addr;
    rt_entry->seqnum = packet_data->targNode.seqnum;
    rt_entry->nextHopAddr = packet_data->sender;
    rt_entry->lastUsed = packet_data->timestamp;
    rt_entry->expirationTime = timex_add(packet_data->timestamp, validity_t);
    rt_entry->metricType = packet_data->metricType;
    rt_entry->metric = packet_data->targNode.metric + link_cost;
    rt_entry->state = ROUTE_STATE_ACTIVE;
}
