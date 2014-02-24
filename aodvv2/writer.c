#include <string.h>
#include <stdio.h>
#ifdef RIOT
#include "net_help.h"
#endif

#include "writer.h"

#define ENABLE_DEBUG (1)
#include "debug.h"

/**
 * Writer to create aodvv2 RFC5444 RREQ and RREP messages.
 * Please note that this is work under construction, specifically:
 * - The Packet header data is bullshit and only serves as an example for me 
 **/

static void _cb_rreq_addAddresses(struct rfc5444_writer *wr);
static void _cb_rreq_addMessageHeader(struct rfc5444_writer *wr, struct rfc5444_writer_message *message);

static void _cb_rrep_addAddresses(struct rfc5444_writer *wr);
static void _cb_rrep_addMessageHeader(struct rfc5444_writer *wr, struct rfc5444_writer_message *message);

static void _cb_rerr_addAddresses(struct rfc5444_writer *wr);
static void _cb_rerr_addMessageHeader(struct rfc5444_writer *wr, struct rfc5444_writer_message *message);

static mutex_t writer_mutex;

struct rfc5444_writer writer;
static struct writer_target _target;

static struct unreachable_node* _unreachable_nodes;
static int _num_unreachable_nodes;

static uint8_t _msg_buffer[128];
static uint8_t _msg_addrtlvs[1000];
static uint8_t _packet_buffer[128];

static struct rfc5444_writer_message *_rreq_msg;
static struct rfc5444_writer_message *_rrep_msg;
static struct rfc5444_writer_message *_rerr_msg;

/*
 * message content provider that will add message TLVs,
 * addresses and address block TLVs to all messages of type RREQ.
 */
static struct rfc5444_writer_content_provider _rreq_message_content_provider = {
    .msg_type = RFC5444_MSGTYPE_RREQ,
    .addAddresses = _cb_rreq_addAddresses,
};

/* declaration of all address TLVs added to the RREQ message */
static struct rfc5444_writer_tlvtype _rreq_addrtlvs[] = {
    [RFC5444_MSGTLV_ORIGSEQNUM] = { .type = RFC5444_MSGTLV_ORIGSEQNUM },
    [RFC5444_MSGTLV_METRIC] = { .type = AODVV2_DEFAULT_METRIC_TYPE },
};

/*
 * message content provider that will add message TLVs,
 * addresses and address block TLVs to all messages of type RREQ.
 */
static struct rfc5444_writer_content_provider _rrep_message_content_provider = {
    .msg_type = RFC5444_MSGTYPE_RREP,
    .addAddresses = _cb_rrep_addAddresses,
};

/* declaration of all address TLVs added to the RREP message */
static struct rfc5444_writer_tlvtype _rrep_addrtlvs[] = {
    [RFC5444_MSGTLV_ORIGSEQNUM] = { .type = RFC5444_MSGTLV_ORIGSEQNUM},
    [RFC5444_MSGTLV_TARGSEQNUM] = { .type = RFC5444_MSGTLV_TARGSEQNUM},
    [RFC5444_MSGTLV_METRIC] = { .type = AODVV2_DEFAULT_METRIC_TYPE },
};

/*
 * message content provider that will add message TLVs,
 * addresses and address block TLVs to all messages of type RREQ.
 */
static struct rfc5444_writer_content_provider _rerr_message_content_provider = {
    .msg_type = RFC5444_MSGTYPE_RERR,
    .addAddresses = _cb_rerr_addAddresses,
};

/* declaration of all address TLVs added to the RREP message */
static struct rfc5444_writer_tlvtype _rerr_addrtlvs[] = {
    [RFC5444_MSGTLV_UNREACHABLE_NODE_SEQNUM] = { .type = RFC5444_MSGTLV_UNREACHABLE_NODE_SEQNUM},
};

/**
 * Callback to define the message header for a RFC5444 RREQ message
 * @param wr
 * @param message
 */
static void
_cb_rreq_addMessageHeader(struct rfc5444_writer *wr, struct rfc5444_writer_message *message)
{
    DEBUG("[aodvv2] %s()\n", __func__);

    /* no originator, no hopcount, has hoplimit, no seqno */
    rfc5444_writer_set_msg_header(wr, message, false, false, true, false);
    rfc5444_writer_set_msg_hoplimit(wr, message, _target.packet_data.hoplimit);
}

/**
 * Callback to add addresses and address TLVs to a RFC5444 RREQ message
 * @param wr
 */
static void
_cb_rreq_addAddresses(struct rfc5444_writer *wr)
{
    DEBUG("[aodvv2] %s()\n", __func__);

    struct rfc5444_writer_address *origNode_addr, *targNode_addr;
    
    /* add origNode address (has no address tlv); is mandatory address */
    origNode_addr = rfc5444_writer_add_address(wr, _rreq_message_content_provider.creator, 
                                               &_target.packet_data.origNode.addr, true);

    /* add origNode address (has no address tlv); is mandatory address */
    targNode_addr = rfc5444_writer_add_address(wr, _rreq_message_content_provider.creator, 
                                               &_target.packet_data.targNode.addr, true);

    /* add SeqNum TLV and metric TLV to origNode */
    // TODO: allow_dup true or false?
    rfc5444_writer_add_addrtlv(wr, origNode_addr, &_rreq_addrtlvs[RFC5444_MSGTLV_ORIGSEQNUM], 
                               &_target.packet_data.origNode.seqNum, 
                               sizeof(_target.packet_data.origNode.seqNum), false);
    rfc5444_writer_add_addrtlv(wr, origNode_addr, &_rreq_addrtlvs[RFC5444_MSGTLV_METRIC],
                               &_target.packet_data.origNode.metric, 
                               sizeof(_target.packet_data.origNode.metric), false);
}

/**
 * Callback to define the message header for a RFC5444 RREQ message
 * @param wr
 * @param message
 */
static void
_cb_rrep_addMessageHeader(struct rfc5444_writer *wr, struct rfc5444_writer_message *message)
{
    DEBUG("[aodvv2] %s()\n", __func__);

    /* no originator, no hopcount, has hoplimit, no seqno */
    rfc5444_writer_set_msg_header(wr, message, false, false, true, false);
    rfc5444_writer_set_msg_hoplimit(wr, message, AODVV2_MAX_HOPCOUNT);
}

/**
 * Callback to add addresses and address TLVs to a RFC5444 RREQ message
 * @param wr
 */
static void
_cb_rrep_addAddresses(struct rfc5444_writer *wr)
{
    DEBUG("[aodvv2] %s()\n", __func__);

    struct rfc5444_writer_address *origNode_addr, *targNode_addr;

    uint16_t origNode_seqNum = _target.packet_data.origNode.seqNum;
    
    uint16_t targNode_seqNum = seqNum_get();
    seqNum_inc();

    uint8_t targNode_hopCt = _target.packet_data.targNode.metric; // TODO -- stimmt das so?! sollte ich da nicht von vore anfangen?!

    /* add origNode address (has no address tlv); is mandatory address */
    origNode_addr = rfc5444_writer_add_address(wr, _rrep_message_content_provider.creator, &_target.packet_data.origNode.addr, true);

    /* add origNode address (has no address tlv); is mandatory address */
    targNode_addr = rfc5444_writer_add_address(wr, _rrep_message_content_provider.creator, &_target.packet_data.targNode.addr, true);

    /* add OrigNode and TargNode SeqNum TLVs */
    // TODO: allow_dup true or false?
    rfc5444_writer_add_addrtlv(wr, origNode_addr, &_rrep_addrtlvs[RFC5444_MSGTLV_ORIGSEQNUM], &origNode_seqNum, sizeof(origNode_seqNum), false);
    rfc5444_writer_add_addrtlv(wr, targNode_addr, &_rrep_addrtlvs[RFC5444_MSGTLV_TARGSEQNUM], &targNode_seqNum, sizeof(targNode_seqNum), false);

    /* Add Metric TLV to targNode Address */
    rfc5444_writer_add_addrtlv(wr, targNode_addr, &_rrep_addrtlvs[RFC5444_MSGTLV_METRIC], &targNode_hopCt, sizeof(targNode_hopCt), false);
}

/**
 * Callback to define the message header for a RFC5444 RREQ message
 * @param wr
 * @param message
 */
static void
_cb_rerr_addMessageHeader(struct rfc5444_writer *wr, struct rfc5444_writer_message *message)
{
    DEBUG("[aodvv2] %s()\n", __func__);

    /* no originator, no hopcount, has hoplimit, no seqno */
    rfc5444_writer_set_msg_header(wr, message, false, false, true, false);
    rfc5444_writer_set_msg_hoplimit(wr, message, _target.packet_data.hoplimit);
}

/**
 * Callback to add addresses and address TLVs to a RFC5444 RERR message
 * @param wr
 */
static void 
_cb_rerr_addAddresses(struct rfc5444_writer *wr)
{
    DEBUG("[aodvv2] %s()\n", __func__);

    struct rfc5444_writer_address *unreachableNode_addr;

    for (int i = 0; i < _num_unreachable_nodes; i++){
        /* add origNode address (has no address tlv); is mandatory address */
        unreachableNode_addr = rfc5444_writer_add_address(wr, _rerr_message_content_provider.creator, 
                                                   &_unreachable_nodes[i].addr, true);

        /* add SeqNum TLV to unreachableNode */
        // TODO: allow_dup true or false?
        rfc5444_writer_add_addrtlv(wr, unreachableNode_addr, &_rerr_addrtlvs[RFC5444_MSGTLV_UNREACHABLE_NODE_SEQNUM], 
                                   &_unreachable_nodes[i].seqnum, 
                                   sizeof(_unreachable_nodes[i].seqnum), false);
    }
}

void writer_init(write_packet_func_ptr ptr)
{
    DEBUG("[aodvv2] %s()\n", __func__);

    mutex_init(&writer_mutex);

    /* define interface for generating rfc5444 packets */
    _target.interface.packet_buffer = _packet_buffer;
    _target.interface.packet_size = sizeof(_packet_buffer);
    
    /* set function to send binary packet content */
    _target.interface.sendPacket = ptr;
    
    /* define the rfc5444 writer */
    writer.msg_buffer = _msg_buffer;
    writer.msg_size = sizeof(_msg_buffer);
    writer.addrtlv_buffer = _msg_addrtlvs;
    writer.addrtlv_size = sizeof(_msg_addrtlvs);

    /* initialize writer */
    rfc5444_writer_init(&writer);

    /* register a target (for sending messages to) in writer */
    rfc5444_writer_register_target(&writer, &_target.interface);

    /* register a message content providers for RREQ and RREP */
    rfc5444_writer_register_msgcontentprovider(&writer, &_rreq_message_content_provider, _rreq_addrtlvs, ARRAYSIZE(_rreq_addrtlvs));
    rfc5444_writer_register_msgcontentprovider(&writer, &_rrep_message_content_provider, _rrep_addrtlvs, ARRAYSIZE(_rrep_addrtlvs));
    rfc5444_writer_register_msgcontentprovider(&writer, &_rerr_message_content_provider, _rerr_addrtlvs, ARRAYSIZE(_rerr_addrtlvs));

    /* register rreq and rrep messages with 16 byte (ipv6) addresses.
       AddPacketHeader & addMessageHeader callbacks are triggered here. */
    _rreq_msg = rfc5444_writer_register_message(&writer, RFC5444_MSGTYPE_RREQ, false, RFC5444_MAX_ADDRLEN);
    _rrep_msg = rfc5444_writer_register_message(&writer, RFC5444_MSGTYPE_RREP, false, RFC5444_MAX_ADDRLEN);
    _rerr_msg = rfc5444_writer_register_message(&writer, RFC5444_MSGTYPE_RERR, false, RFC5444_MAX_ADDRLEN);

    _rreq_msg->addMessageHeader = _cb_rreq_addMessageHeader;
    _rrep_msg->addMessageHeader = _cb_rrep_addMessageHeader;
    _rerr_msg->addMessageHeader = _cb_rerr_addMessageHeader;
}

/**
 * Send a RREQ.
 * @param na_origNode
 * @param na_targNode
 * @param next_hop Address the RREQ is sent to (i.e. appropriate Multicast group) 
 */
void writer_send_rreq(struct netaddr* na_origNode, struct netaddr* na_targNode, struct netaddr* next_hop)
{
    DEBUG("[aodvv2] %s()\n", __func__);

    if (na_origNode == NULL || na_targNode == NULL || next_hop == NULL)
        return;

    /* Make sure no other thread is using the writer right now */
    if (mutex_lock(&writer_mutex) == 1) {

        memset(&_target.packet_data, 0, sizeof(struct aodvv2_packet_data));
        memcpy(&_target.packet_data.origNode.addr, na_origNode, sizeof (struct netaddr));
        memcpy(&_target.packet_data.targNode.addr, na_targNode, sizeof (struct netaddr));
        _target.packet_data.hoplimit = AODVV2_MAX_HOPCOUNT;
        _target.type = RFC5444_MSGTYPE_RREQ;

        /* set address to which the write_packet callback should send our RREQ */
        memcpy(&_target.target_address, next_hop, sizeof (struct netaddr));

        /* set info about the rreq so the callbacks just have to grab it from packet_data */
        uint16_t origNode_seqNum = seqNum_get();
        _target.packet_data.origNode.seqNum = origNode_seqNum;
        seqNum_inc();

        _target.packet_data.origNode.metric = 0;

        rfc5444_writer_create_message_alltarget(&writer, RFC5444_MSGTYPE_RREQ);
        rfc5444_writer_flush(&writer, &_target.interface, false);
        mutex_unlock(&writer_mutex);
    } // TODO: handle mutex_lock() = -1?  
}


/**
 * just forward a RREQ, don't change anything.
 */
void writer_forward_rreq(struct aodvv2_packet_data* packet_data, struct netaddr* next_hop)
{
    DEBUG("[aodvv2] %s()\n", __func__);

    if (packet_data == NULL || next_hop == NULL)
        return;

    /* Make sure no other thread is using the writer right now */
    if (mutex_lock(&writer_mutex) == 1) {

        memcpy(&_target.packet_data, packet_data, sizeof(struct aodvv2_packet_data));
        _target.type = RFC5444_MSGTYPE_RREQ;

        /* set address to which the write_packet callback should send our RREQ */
        memcpy(&_target.target_address, next_hop, sizeof (struct netaddr));
        
        rfc5444_writer_create_message_alltarget(&writer, RFC5444_MSGTYPE_RREQ);
        rfc5444_writer_flush(&writer, &_target.interface, false);
        mutex_unlock(&writer_mutex);
    } // TODO: handle mutex_lock() = -1?  
}

/**
 * Send a RREP.
 * @param na_origNode
 * @param na_targNode
 * @param next_hop Address the RREP is sent to 
 */
void writer_send_rrep(struct aodvv2_packet_data* packet_data, struct netaddr* next_hop)
{
    DEBUG("[aodvv2] %s()\n", __func__);

    if (packet_data == NULL || next_hop == NULL)
        return;

    /* Make sure no other thread is using the writer right now */
    if (mutex_lock(&writer_mutex) == 1) {

        memcpy(&_target.packet_data, packet_data, sizeof(struct aodvv2_packet_data));
        _target.type = RFC5444_MSGTYPE_RREP;

        /* set address to which the write_packet callback should send our RREQ */
        memcpy(&_target.target_address, next_hop, sizeof (struct netaddr));

        rfc5444_writer_create_message_alltarget(&writer, RFC5444_MSGTYPE_RREP);
        rfc5444_writer_flush(&writer, &_target.interface, false);
        mutex_unlock(&writer_mutex);
    } // TODO: handle mutex_lock() = -1?  
}

void writer_send_rerr(struct unreachable_node unreachable_nodes[], int len, int hoplimit, struct netaddr* next_hop)
{
    DEBUG("[aodvv2] %s()\n", __func__);

    if (unreachable_nodes == NULL || next_hop == NULL)
        return;

    if (mutex_lock(&writer_mutex) == 1) {

        _target.packet_data.hoplimit = hoplimit;
        _target.type = RFC5444_MSGTYPE_RERR;
        _unreachable_nodes = unreachable_nodes;
        _num_unreachable_nodes = len;

        /* set address to which the write_packet callback should send our RREQ */
        memcpy(&_target.target_address, next_hop, sizeof (struct netaddr));

        rfc5444_writer_create_message_alltarget(&writer, RFC5444_MSGTYPE_RERR);
        rfc5444_writer_flush(&writer, &_target.interface, false);
        mutex_unlock(&writer_mutex);
    }
}

void writer_cleanup(void)
{
    DEBUG("[aodvv2] %s()\n", __func__);
    rfc5444_writer_cleanup(&writer);
}
