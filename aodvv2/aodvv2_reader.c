#include <string.h>
#include <stdio.h>
#ifdef RIOT
#include "net_help.h"
#endif
#include "common/common_types.h"
#include "common/netaddr.h"
#include "rfc5444/rfc5444_reader.h"

#include "include/aodvv2.h"
#include "aodvv2_reader.h"
#include "routing.h"

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
bool offers_improvement(struct aodvv2_routing_entry_t* rt_entry, struct node_data* node_data);
uint8_t link_cost(uint8_t metricType, struct aodvv2_packet_data* data);
uint8_t max_metric(uint8_t metricType);

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
    [RFC5444_MSGTLV_SEQNUM] = { .type = RFC5444_MSGTLV_SEQNUM }, 
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
    bool is_origNode_addr;

    printf("[aodvv2] %s()\n", __func__);
    printf("\tmessage type: %d\n", cont->type);
    printf("\taddr: %s\n", netaddr_to_string(&nbuf, &cont->addr));

    /* handle SeqNum TLV */
    tlv = _rreq_rrep_address_consumer_entries[RFC5444_MSGTLV_SEQNUM].tlv;
    if (!tlv) {
        /* assume that tlv missing => targNode Address */
        is_origNode_addr = false;
        packet_data.targNode.addr = cont->addr;
        packet_data.targNode.prefixlen = cont->addr._prefix_len; 
    }
    else {
        if (tlv->type_ext == RFC5444_MSGTLV_ORIGNODE_SEQNUM) {
            printf("\ttlv RFC5444_MSGTLV_SEQNUM: %d exttype: %d\n", *tlv->single_value, tlv->type_ext );
            is_origNode_addr = true;
            packet_data.origNode.addr = cont->addr;
            packet_data.origNode.seqNum = *tlv->single_value;
            packet_data.origNode.prefixlen = cont->addr._prefix_len; 
        } else if (tlv->type_ext == RFC5444_MSGTLV_TARGNODE_SEQNUM) {
            printf("\ttlv RFC5444_MSGTLV_SEQNUM: %d exttype: %d\n", *tlv->single_value, tlv->type_ext );
            is_origNode_addr = false;
            packet_data.targNode.addr = cont->addr;
            packet_data.targNode.seqNum = *tlv->single_value;
            packet_data.targNode.prefixlen = cont->addr._prefix_len; 
        } else {
            printf("ERROR: illegal extension type.\n");
            return RFC5444_DROP_PACKET;
        }
    }

    /* handle Metric TLV */
    tlv = _rreq_rrep_address_consumer_entries[RFC5444_MSGTLV_METRIC].tlv;
    if (!tlv && is_origNode_addr){
        printf("\tERROR: Missing or unknown metric TLV.\n");
        return RFC5444_DROP_PACKET;
    }
    if (tlv) {
        if (!is_origNode_addr){
            printf("\tERROR: Metric TLV belongs to wrong address.\n");
            return RFC5444_DROP_PACKET;
        }
        printf("\ttlv RFC5444_MSGTLV_METRIC hopCt: %d, type: %d\n", *tlv->single_value, tlv->type);
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
    printf("[aodvv2] %s()\n", __func__);

    if (!cont->has_hoplimit) {
        printf("ERROR: missing hop limit\n");
        return RFC5444_DROP_PACKET;
    }

    printf("[aodvv2] %s()\n\t i can has hop limit: %d\n",__func__ , cont->hoplimit);
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
static enum rfc5444_result _cb_rreq_end_callback(
    struct rfc5444_reader_tlvblock_context *cont, bool dropped)
{
    struct aodvv2_routing_entry_t* rt_entry;
    timex_t now;

    printf("[aodvv2] %s() dropped: %d\n", __func__, dropped);

    /* Check if packet contains the rquired information */
    if (dropped) {
        printf("\tDropping packet.\n");
        return RFC5444_DROP_PACKET;
    } 
    if ((packet_data.origNode.addr._type == AF_UNSPEC) || !packet_data.origNode.seqNum) {
        printf("\tERROR: missing OrigNode Address or SeqNum. Dropping packet.\n");
        return RFC5444_DROP_PACKET;
    }
    if (packet_data.targNode.addr._type == AF_UNSPEC) {
        printf("\tERROR: missing TargNode Address. Dropping packet.\n");
        return RFC5444_DROP_PACKET; 
    }
    if (packet_data.hoplimit == 0) {
        printf("\tERROR: Hoplimit is 0. Dropping packet.\n");
        return RFC5444_DROP_PACKET;
    }

    packet_data.hoplimit-- ;
 
    /* for every relevant
     * address (RteMsg.Addr) in the RteMsg, HandlingRtr searches its route
     * table to see if there is a route table entry with the same MetricType
     * of the RteMsg, matching RteMsg.Addr.
     */

    rt_entry = get_routing_entry(&packet_data.origNode.addr);

    if (!rt_entry || (rt_entry->metricType != packet_data.metricType)){
    //if(true){
        printf("\tCreating new Routing Table entry...\n");
        /* de-NULL rt_entry */
        rt_entry = (struct aodvv2_routing_entry_t*)malloc(sizeof(struct aodvv2_routing_entry_t));
        memset(rt_entry, 0, sizeof(rt_entry));
        /* add empty rt_entry so that we can fill it later */
        add_routing_entry(rt_entry);
    } else {
        if (!offers_improvement(rt_entry, &packet_data.origNode.addr)){
            printf("\tPacket offers no improvement over known route. Dropping Packet.\n");
            return RFC5444_DROP_PACKET; 
        }
    }
    
    printf("old entry:\n");
    print_rt_entry(rt_entry);
    /* The incoming routing information is better than existing routing 
     * table information and SHOULD be used to improve the route table. */ 
    rt_entry->address = packet_data.origNode.addr;
    rt_entry->prefixlen = packet_data.origNode.prefixlen;
    rt_entry->seqNum = packet_data.origNode.seqNum;
    rt_entry->nextHopAddress = packet_data.sender;
    rt_entry->lastUsed = now;
    rt_entry->expirationTime = timex_add(now, validity_t);
    rt_entry->broken = false;
    rt_entry->metricType = packet_data.metricType;
    rt_entry->metric = packet_data.origNode.metric + 1; // TODO: determine cost(L) properly
    // TODO: state 

    printf("new entry:\n");
    print_rt_entry(rt_entry);
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

    printf("[aodvv2] %s()\n", __func__);
    printf("\tmessage type: %d\n", cont->type);
    printf("\taddr: %s\n", netaddr_to_string(&nbuf, &cont->addr));

    /* handle SeqNum TLV */
    tlv = _rreq_rrep_address_consumer_entries[RFC5444_MSGTLV_SEQNUM].tlv;
    if (!tlv) {
        printf("ERROR: missing SeqNum TLV.\n");
        return RFC5444_DROP_PACKET;
    }
    else {
        if (tlv->type_ext == RFC5444_MSGTLV_ORIGNODE_SEQNUM) {
            printf("\ttlv RFC5444_MSGTLV_SEQNUM: %d exttype: %d\n", *tlv->single_value, tlv->type_ext );
            is_targNode_addr = false;
            packet_data.origNode.addr = cont->addr;
            packet_data.origNode.seqNum = *tlv->single_value;
            packet_data.origNode.prefixlen = cont->addr._prefix_len; 
        } else if (tlv->type_ext == RFC5444_MSGTLV_TARGNODE_SEQNUM) {
            printf("\ttlv RFC5444_MSGTLV_SEQNUM: %d exttype: %d\n", *tlv->single_value, tlv->type_ext );
            is_targNode_addr = true;
            packet_data.targNode.addr = cont->addr;
            packet_data.targNode.seqNum = *tlv->single_value;
            packet_data.targNode.prefixlen = cont->addr._prefix_len; 
        } else {
            printf("ERROR: illegal extension type.\n");
            return RFC5444_DROP_PACKET;
        }
    }

    /* handle Metric TLV */
    tlv = _rreq_rrep_address_consumer_entries[RFC5444_MSGTLV_METRIC].tlv;
    if (!tlv && is_targNode_addr){
        printf("\tERROR: Missing or unknown metric TLV.\n");
        return RFC5444_DROP_PACKET;
    }
    if (tlv) {
        if (!is_targNode_addr){
            printf("\tERROR: metric TLV belongs to wrong address.\n");
            return RFC5444_DROP_PACKET;
        }
        printf("\ttlv RFC5444_MSGTLV_METRIC hopCt: %d, type: %d\n", *tlv->single_value, tlv->type);
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
    printf("[aodvv2] %s()\n", __func__);

    if (!cont->has_hoplimit) {
        printf("\tERROR: missing hop limit\n");
        return RFC5444_DROP_PACKET;
    }

    printf("[aodvv2] %s()\n\t i can has hop limit: %d\n",__func__ , cont->hoplimit);
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
    
    printf("[aodvv2] %s() dropped: %d\n", __func__, dropped);

    /* Check if packet contains the rquired information */
    if (dropped) {
        printf("\tDropping packet.\n");
        return RFC5444_DROP_PACKET;
    } 
    if ((packet_data.origNode.addr._type == AF_UNSPEC) || !packet_data.origNode.seqNum) {
        printf("\tERROR: missing OrigNode Address or SeqNum. Dropping packet.\n");
        return RFC5444_DROP_PACKET;
    }
    if ((packet_data.targNode.addr._type == AF_UNSPEC) || !packet_data.targNode.seqNum) {
        printf("\tERROR: missing TargNode Address or SeqNum. Dropping packet.\n");
        return RFC5444_DROP_PACKET; 
    }
    if (packet_data.hoplimit == 0) {
        printf("\tERROR: Hoplimit is 0. Dropping packet.\n");
        return RFC5444_DROP_PACKET;
    }
    packet_data.hoplimit-- ;

    /* for every relevant
     * address (RteMsg.Addr) in the RteMsg, HandlingRtr searches its route
     * table to see if there is a route table entry with the same MetricType
     * of the RteMsg, matching RteMsg.Addr.
     */

    rt_entry = get_routing_entry(&packet_data.targNode.addr);

    if (!rt_entry || (rt_entry->metricType != packet_data.metricType)){
    //if(true){
        printf("\tCreating new Routing Table entry...\n");
        /* de-NULL rt_entry */
        rt_entry = (struct aodvv2_routing_entry_t*)malloc(sizeof(struct aodvv2_routing_entry_t));
        memset(rt_entry, 0, sizeof(rt_entry));
        /* add empty rt_entry so that we can fill it later */
        add_routing_entry(rt_entry);
    } else {
        if (!offers_improvement(rt_entry, &packet_data.targNode.addr)){
            printf("\tPacket offers no improvement over known route. Dropping Packet.\n");
            return RFC5444_DROP_PACKET; 
        }
    }

}

void reader_init(void)
{
    printf("%s()\n", __func__);

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
    printf("[aodvv2] %s() TODO\n", __func__);
    rfc5444_reader_cleanup(&reader);
}

/**
 * 
 * @param sender Address of the node from which the packet was received (TODO: should this be a pointer?)
 */
int reader_handle_packet(void* buffer, size_t length, struct netaddr sender)
{
    packet_data.sender = sender;
    return rfc5444_reader_handle_packet(&reader, buffer, length);
}

//============= HELPER FUNCTIONS ===============================================

/*
 * handle collected data as described in Section 6.1 
 */
bool offers_improvement(struct aodvv2_routing_entry_t* rt_entry, struct node_data* node_data)
{
    /* Check if new info is stale */    
    if (node_data->seqNum < rt_entry->seqNum )
        return false;
    /* Check if new info is more costly */
    if ((node_data->metric >= rt_entry->metric) && !(rt_entry->broken))
        return false;
    return true;
}

/*
 * Cost(L): Get Cost of a Link regarding the specified metric.
 * (currently only AODVV2_DEFAULT_METRIC_TYPE (HopCt) immplemented)
 * returns cost if metric is known, NULL otherwise
 */
uint8_t link_cost(uint8_t metricType, struct aodvv2_packet_data* data)
{
    if (metricType == AODVV2_DEFAULT_METRIC_TYPE)
        return 1;
    return NULL;
}

/*
 * MEX_METRIC[MetricType]:
 * returns maximum value of the given metric if metric is known, NULL otherwise.
 */
uint8_t max_metric(uint8_t metricType)
{
    if (metricType == AODVV2_DEFAULT_METRIC_TYPE)
        return AODVV2_MAX_HOPCOUNT;
    return NULL;
}

