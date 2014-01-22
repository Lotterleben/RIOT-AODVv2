
#ifdef RIOT
#include "net_help.h"
#endif

#include "reader.h"

#define ENABLE_DEBUG (1)
#include "debug.h"

/* This is where we store data gathered from packets */
static struct aodvv2_packet_data packet_data;

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

/* helper functions */
static bool _offers_improvement(struct aodvv2_routing_entry_t* rt_entry, struct node_data* node_data);
static uint8_t _get_link_cost(uint8_t metricType, struct aodvv2_packet_data* data);
static uint8_t _get_max_metric(uint8_t metricType);
static uint8_t _get_updated_metric(uint8_t metricType, uint8_t metric);

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
 * Address consumer entries definition
 * TLV types RFC5444_MSGTLV__SEQNUM and RFC5444_MSGTLV_METRIC
 */
static struct rfc5444_reader_tlvblock_consumer_entry _rreq_rrep_address_consumer_entries[] = {
    [RFC5444_MSGTLV_ORIGSEQNUM] = { .type = RFC5444_MSGTLV_ORIGSEQNUM},
    [RFC5444_MSGTLV_TARGSEQNUM] = { .type = RFC5444_MSGTLV_TARGSEQNUM},
    [RFC5444_MSGTLV_METRIC] = { .type = AODVV2_DEFAULT_METRIC_TYPE }
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
        packet_data.origNode.seqNum = *tlv->single_value;
        packet_data.origNode.prefixlen = cont->addr._prefix_len;         
    }

    /* handle TargNode SeqNum TLV */
    tlv = _rreq_rrep_address_consumer_entries[RFC5444_MSGTLV_TARGSEQNUM].tlv;
    if (tlv) {
        DEBUG("\ttlv RFC5444_MSGTLV_TARGSEQNUM: %d\n", *tlv->single_value);
        is_targNode_addr = true;
        packet_data.targNode.addr = cont->addr;
        packet_data.targNode.seqNum = *tlv->single_value;
        packet_data.targNode.prefixlen = cont->addr._prefix_len;         
    }
    if (!tlv && !is_origNode_addr) {
        /* assume that tlv missing => targNode Address */
        is_targNode_addr = true;
        packet_data.targNode.addr = cont->addr;
        packet_data.targNode.prefixlen = cont->addr._prefix_len; 
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

    DEBUG("[aodvv2] %s()\n\t i can has hop limit: %d\n",__func__ , cont->hoplimit);
    packet_data.hoplimit = cont->hoplimit;
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
    if ((packet_data.origNode.addr._type == AF_UNSPEC) || !packet_data.origNode.seqNum){
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
    if (rreq_is_redundant(&packet_data)){
        DEBUG("\tPacket is redundant. Dropping Packet. %i\n", RFC5444_DROP_PACKET);
        return RFC5444_DROP_PACKET;
    }

    packet_data.origNode.metric = _get_updated_metric(packet_data.metricType, packet_data.origNode.metric);
    rtc_time(&now);
    packet_data.timestamp = now;

    /* for every relevant
     * address (RteMsg.Addr) in the RteMsg, HandlingRtr searches its route
     * table to see if there is a route table entry with the same MetricType
     * of the RteMsg, matching RteMsg.Addr.
     */

    rt_entry = routingtable_get_entry(&packet_data.origNode.addr, packet_data.metricType);

    if (!rt_entry || (rt_entry->metricType != packet_data.metricType)){
        DEBUG("\tCreating new Routing Table entry...\n");
        /* de-NULL rt_entry */
        rt_entry = (struct aodvv2_routing_entry_t*)malloc(sizeof(struct aodvv2_routing_entry_t));
        memset(rt_entry, 0, sizeof(rt_entry)); // nullt nicht, sondern amcht uint8_ts zu 48s.. o0 TODO
        /* add empty rt_entry so that we can fill it later */
        routingtable_add_entry(rt_entry);
    } else {
        if (!_offers_improvement(rt_entry, &packet_data.origNode.addr)){
            DEBUG("\tPacket offers no improvement over known route. Dropping Packet.\n");
            return RFC5444_DROP_PACKET; 
        }
    }
    
    //DEBUG("old entry:\n");
    //print_rt_entry(rt_entry);
    /* The incoming routing information is better than existing routing 
     * table information and SHOULD be used to improve the route table. */ 
    rt_entry->address = packet_data.origNode.addr;
    rt_entry->prefixlen = packet_data.origNode.prefixlen;
    rt_entry->seqNum = packet_data.origNode.seqNum;
    rt_entry->nextHopAddress = packet_data.sender;
    rt_entry->lastUsed = packet_data.timestamp;
    rt_entry->expirationTime = timex_add(packet_data.timestamp, validity_t);
    rt_entry->broken = false;
    rt_entry->metricType = packet_data.metricType;
    rt_entry->metric = packet_data.origNode.metric + link_cost;
    rt_entry->state = ROUTE_STATE_ACTIVE;

    //DEBUG("new entry:\n");
    //print_rt_entry(rt_entry);

    /*
     * If TargNode is a client of the router receiving the RREQ, then the
     * router generates a RREP message as specified in Section 7.4, and
     * subsequently processing for the RREQ is complete.  Otherwise,
     * processing continues as follows.
     */

    if (is_client(&packet_data.targNode.addr, packet_data.targNode.prefixlen)){
        DEBUG("[aodvv2] TargNode is in client list, sending RREP\n");    
        //send_rrep(&packet_data, &packet_data.sender);  
        return RFC5444_OKAY;
    }

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
        packet_data.targNode.seqNum = *tlv->single_value;
        packet_data.targNode.prefixlen = cont->addr._prefix_len;
    }

    /* handle OrigNode SeqNum TLV */
    tlv = _rreq_rrep_address_consumer_entries[RFC5444_MSGTLV_ORIGSEQNUM].tlv;
    if (tlv) {
        DEBUG("\ttlv RFC5444_MSGTLV_ORIGSEQNUM: %d\n", *tlv->single_value);
        is_targNode_addr = false;
        packet_data.origNode.addr = cont->addr;
        packet_data.origNode.seqNum = *tlv->single_value;
        packet_data.origNode.prefixlen = cont->addr._prefix_len;
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

    DEBUG("[aodvv2] %s()\n\t i can has hop limit: %d\n",__func__ , cont->hoplimit);
    packet_data.hoplimit = cont->hoplimit;
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

    /* Check if packet contains the rquired information */
    if (dropped) {
        DEBUG("\t Dropping packet.\n");
        return RFC5444_DROP_PACKET;
    } 
    if ((packet_data.origNode.addr._type == AF_UNSPEC) || !packet_data.origNode.seqNum) {
        DEBUG("\tERROR: missing OrigNode Address or SeqNum. Dropping packet.\n");
        return RFC5444_DROP_PACKET;
    }
    if ((packet_data.targNode.addr._type == AF_UNSPEC) || !packet_data.targNode.seqNum) {
        DEBUG("\tERROR: missing TargNode Address or SeqNum. Dropping packet.\n");
        return RFC5444_DROP_PACKET; 
    }
    if (packet_data.hoplimit == 0) {
        DEBUG("\tERROR: Hoplimit is 0. Dropping packet.\n");
        return RFC5444_DROP_PACKET;
    }
    if ((_get_max_metric(packet_data.metricType) - link_cost) <= packet_data.targNode.metric){
        DEBUG("\tMetric Limit reached. Dropping packet.\n");
        return RFC5444_DROP_PACKET;
    }

    packet_data.origNode.metric = _get_updated_metric(packet_data.metricType, packet_data.origNode.metric);
    rtc_time(&now);
    packet_data.timestamp = now;

    /* for every relevant
     * address (RteMsg.Addr) in the RteMsg, HandlingRtr searches its route
     * table to see if there is a route table entry with the same MetricType
     * of the RteMsg, matching RteMsg.Addr.
     */

    rt_entry = routingtable_get_entry(&packet_data.targNode.addr, packet_data.metricType);

    if (!rt_entry || (rt_entry->metricType != packet_data.metricType)){
        DEBUG("\tCreating new Routing Table entry...\n");
        /* de-NULL rt_entry */
        rt_entry = (struct aodvv2_routing_entry_t*)malloc(sizeof(struct aodvv2_routing_entry_t));
        memset(rt_entry, 0, sizeof(rt_entry)); // nullt nicht, sondern macht uint8_ts zu 48s.. o0 TODO
        /* add empty rt_entry so that we can fill it later */
        routingtable_add_entry(rt_entry);
    } else {
        if (!_offers_improvement(rt_entry, &packet_data.targNode.addr)) {
            DEBUG("\tPacket offers no improvement over known route. Dropping Packet.\n");
            return RFC5444_DROP_PACKET; 
        }
    }
    
    //DEBUG("old entry:\n");
    //print_rt_entry(rt_entry);
    /* The incoming routing information is better than existing routing 
     * table information and SHOULD be used to improve the route table. */ 
    rt_entry->address = packet_data.targNode.addr;
    rt_entry->prefixlen = packet_data.targNode.prefixlen;
    rt_entry->seqNum = packet_data.targNode.seqNum;
    rt_entry->nextHopAddress = packet_data.sender;
    rt_entry->lastUsed = packet_data.timestamp;
    rt_entry->expirationTime = timex_add(packet_data.timestamp, validity_t);
    rt_entry->broken = false;
    rt_entry->metricType = packet_data.metricType;
    rt_entry->metric = packet_data.targNode.metric + link_cost;
    rt_entry->state = ROUTE_STATE_ACTIVE;

    //DEBUG("new entry:\n");
    //print_rt_entry(rt_entry);

    /*
     * If HandlingRtr is RREQ_Gen then the RREP satisfies RREQ_Gen's
     * earlier RREQ, and RREP processing is completed.  Any packets
     * buffered for OrigNode should be transmitted.
     */

    if (is_client(&packet_data.origNode.addr, packet_data.origNode.prefixlen)){
        DEBUG("This is my RREP. We are done here, thanks!\n");
        // TODO : transmit buffered data    
        return RFC5444_DROP_PACKET;
    }
    /* If HandlingRtr is not RREQ_Gen then the outgoing RREP is sent to the
      Route.NextHopAddress for the RREP.AddrBlk[OrigNodeNdx].   
     */
    else {
        DEBUG("[aodvv2] Not my RREP, passing it on to the next hop\n");
        //struct netaddr* next_hop = get_next_hop(&packet_data.origNode.addr, packet_data.metricType);
        //send_rrep(&packet_data, &next_hop);
    }
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

    /* register address consumer */
    rfc5444_reader_add_message_consumer(&reader, &_rreq_address_consumer,
        _rreq_rrep_address_consumer_entries, ARRAYSIZE(_rreq_rrep_address_consumer_entries));
    rfc5444_reader_add_message_consumer(&reader, &_rrep_address_consumer,
        _rreq_rrep_address_consumer_entries, ARRAYSIZE(_rreq_rrep_address_consumer_entries));
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
    if (cmp_seqnum(node_data->seqNum, rt_entry->seqNum) == -1)
        return false;
    /* Check if new info is more costly */
    if ((node_data->metric >= rt_entry->metric) && !(rt_entry->broken))
        return false;
    return true;
}

/*
 * Cost(L): Get Cost of a Link regarding the specified metric.
 * (currently only AODVV2_DEFAULT_METRIC_TYPE (HopCt) implemented)
 * returns cost if metric is known, NULL otherwise
 */
static uint8_t _get_link_cost(uint8_t metricType, struct aodvv2_packet_data* data)
{
    if (metricType == AODVV2_DEFAULT_METRIC_TYPE)
        return 1;
    return NULL;
}

/*
 * MAX_METRIC[MetricType]:
 * returns maximum value of the given metric if metric is known, NULL otherwise.
 */
static uint8_t _get_max_metric(uint8_t metricType)
{
    if (metricType == AODVV2_DEFAULT_METRIC_TYPE)
        return AODVV2_MAX_HOPCOUNT;
    return NULL;
}

/*
 * Calculate a metric's new value according to the specified MetricType
 * (currently only implemented for AODVV2_DEFAULT_METRIC_TYPE (HopCt))
 */
static uint8_t _get_updated_metric(uint8_t metricType, uint8_t metric)
{
    if (metricType == AODVV2_DEFAULT_METRIC_TYPE)
        return metric++;
    return NULL;
}

