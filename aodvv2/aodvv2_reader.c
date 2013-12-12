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

static enum rfc5444_result _cb_rreq_rrep_blocktlv_addresstlvs_okay(
    struct rfc5444_reader_tlvblock_context *cont);

static enum rfc5444_result _cb_rreq_blocktlv_messagetlvs_okay(
    struct rfc5444_reader_tlvblock_context *cont);
static enum rfc5444_result _cb_rreq_end_callback(
    struct rfc5444_reader_tlvblock_context *cont, bool dropped);

static enum rfc5444_result _cb_rrep_blocktlv_messagetlvs_okay(
    struct rfc5444_reader_tlvblock_context *cont);
static enum rfc5444_result _cb_rrep_end_callback(
    struct rfc5444_reader_tlvblock_context *cont, bool dropped);

static bool offers_improvement(struct aodvv2_routing_entry_t* rt_entry);

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
    .block_callback = _cb_rreq_rrep_blocktlv_addresstlvs_okay,
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
    .block_callback = _cb_rreq_rrep_blocktlv_addresstlvs_okay,
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
 * This block callback is called for every address. Since the basic structure of
 * RREQ and RREP is the same, they both use this callback.
 *
 * @param cont
 * @return
 */
static enum rfc5444_result _cb_rreq_rrep_blocktlv_addresstlvs_okay(struct rfc5444_reader_tlvblock_context *cont)
{
    struct netaddr_str nbuf;
    struct rfc5444_reader_tlvblock_entry* tlv;

    printf("[aodvv2] %s()\n", __func__);
    printf("\tmessage type: %d\n", cont->type);
    printf("\taddr: %s\n", netaddr_to_string(&nbuf, &cont->addr));

    /* handle SeqNum TLV */
    tlv = _rreq_rrep_address_consumer_entries[RFC5444_MSGTLV_SEQNUM].tlv;
    if (!tlv) {
        /* assume that tlv missing => targNode Address */
        packet_data.targNode_addr = cont->addr;
        packet_data.targNode_addr_prefixlen = cont->addr._prefix_len; 
    }
    while (tlv) {
        if (tlv->type_ext == RFC5444_MSGTLV_ORIGNODE_SEQNUM) {
            printf("\ttlv RFC5444_MSGTLV_SEQNUM: %d exttype: %d\n", *tlv->single_value, tlv->type_ext );
            packet_data.origNode_addr = cont->addr;
            packet_data.origNode_seqNum = *tlv->single_value;
            packet_data.origNode_addr_prefixlen = cont->addr._prefix_len; 
        } else if (tlv->type_ext == RFC5444_MSGTLV_TARGNODE_SEQNUM) {
            printf("\ttlv RFC5444_MSGTLV_SEQNUM: %d exttype: %d\n", *tlv->single_value, tlv->type_ext );
            packet_data.targNode_addr = cont->addr;
            packet_data.targNode_seqNum = *tlv->single_value;
            packet_data.targNode_addr_prefixlen = cont->addr._prefix_len; 
        } else {
            printf("ERROR: illegal extension type.\n");
            return RFC5444_DROP_PACKET;
        }
        tlv = tlv->next_entry;
    }

    /* handle Metric TLV */
    tlv = _rreq_rrep_address_consumer_entries[RFC5444_MSGTLV_METRIC].tlv;
    if (!tlv){
        printf("\tERROR: Missing metric TLV.\n");
        return RFC5444_DROP_PACKET;
    }
    while (tlv) {
        printf("\ttlv RFC5444_MSGTLV_METRIC hopCt: %d\n", *tlv->single_value);
        packet_data.metric = *tlv->single_value;
        tlv = tlv->next_entry;
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

    printf("[aodvv2] %s() dropped: %d\n", __func__, dropped);

    /* Check if packet contains the rquired information */
    if (dropped) {
        printf("Dropping packet.\n");
        return RFC5444_DROP_PACKET;
    } 
    if ((packet_data.origNode_addr._type == AF_UNSPEC) || !packet_data.origNode_seqNum) {
        printf("ERROR: missing OrigNode Address or SeqNum. Dropping packet.\n");
        return RFC5444_DROP_PACKET;
    }
    if (packet_data.targNode_addr._type == AF_UNSPEC) {
        printf("ERROR: missing TargNode Address. Dropping packet.\n");
        return RFC5444_DROP_PACKET; 
    }
    if (packet_data.hoplimit == 0) {
        printf("ERROR: Hoplimit is 0. Dropping packet.\n");
        return RFC5444_DROP_PACKET;
    }
    packet_data.hoplimit-- ;
 
    /* for every relevant
     * address (RteMsg.Addr) in the RteMsg, HandlingRtr searches its route
     * table to see if there is a route table entry with the same MetricType
     * of the RteMsg, matching RteMsg.Addr.
     */

    if (rt_entry = get_routing_entry(&packet_data.origNode_addr)) {
        if(!offers_improvement(rt_entry)){
            printf("Packet offers no improvement over known route. Dropping Packet.\n");
            return RFC5444_DROP_PACKET; 
        }

        timex_t now;
        rtc_time(&now);
        
        printf("old entry:\n");
        print_rt_entry(rt_entry);
        /* The incoming routing information is better than existing routing 
         * table information and SHOULD be used to improve the route table. */ 
        rt_entry->address = packet_data.origNode_addr;
        rt_entry->prefixlen = packet_data.origNode_addr_prefixlen;
        rt_entry->seqNum = packet_data.origNode_seqNum;
        rt_entry->nextHopAddress = packet_data.sender;
        rt_entry->lastUsed = now;
        rt_entry->expirationTime = timex_add(now, validity_t);
        rt_entry->broken = false;
        rt_entry->metricType = AODVV2_DEFAULT_METRIC_TYPE; // TODO: deduct from packet
        // TODO: metric (WTF)
        //rt_entry->metric = packet_data.metric + ???; // ??? = Cost(L) ... whatever that means..

        printf("new entry:\n");
        print_rt_entry(rt_entry);

    } else {

    }


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
        printf("ERROR: missing hop limit\n");
        return RFC5444_DROP_PACKET;
    }

    printf("[aodvv2] %s()\n\t i can has hop limit: %d\n",__func__ , cont->hoplimit);
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
    printf("[aodvv2] %s() dropped: %d\n", __func__, dropped);

    /* Check if packet contains the rquired information */
    if (dropped) {
        printf("Dropping packet.\n");
        return RFC5444_DROP_PACKET;
    } 
    if ((packet_data.origNode_addr._type == AF_UNSPEC) || !packet_data.origNode_seqNum) {
        printf("ERROR: missing OrigNode Address or SeqNum. Dropping packet.\n");
        return RFC5444_DROP_PACKET;
    }
    if ((packet_data.targNode_addr._type == AF_UNSPEC) || !packet_data.targNode_seqNum) {
        printf("ERROR: missing TargNode Address. Dropping packet.\n");
        return RFC5444_DROP_PACKET; 
    }
    if (packet_data.hoplimit == 0) {
        printf("ERROR: Hoplimit is 0. Dropping packet.\n");
        return RFC5444_DROP_PACKET;
    }
    packet_data.hoplimit-- ;

    /* handle collected data as described in Section 6.1 */

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
static bool offers_improvement(struct aodvv2_routing_entry_t* rt_entry)
{
    /* Check if new info is stale */    
    if (packet_data.origNode_seqNum < rt_entry->seqNum )
        return false;
    /* Check if new info is more costly */
    if ((packet_data.metric >= rt_entry->metric) && !(rt_entry->broken))
        return false;
    return true;
}
