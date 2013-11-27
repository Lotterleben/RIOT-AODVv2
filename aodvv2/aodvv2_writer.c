#include <string.h>
#include <stdio.h>
#ifdef RIOT
#include "net_help.h"
#endif

#include "debug.h"

#include "common/common_types.h"
#include "common/netaddr.h"
#include "rfc5444/rfc5444_writer.h"
#include "rfc5444/rfc5444_iana.h"

#include "aodvv2_writer.h"
#include "include/aodvv2.h"
#include "sender.h"

/**
 * Writer to create aodvv2 RFC5444 RREQ and RREP messages.
 * Please note that this is work under construction, specifically:
 * - The messageTLVs will be changed into Address TLVs or Address Block TLVs as
 *   soon as Charlie has answered my questions on the subject
 * - The Packet header data is bullshit and only serves as an example for me 
 **/

static void _cb_rreq_addMessageTLVs(struct rfc5444_writer *wr);
static void _cb_rreq_addAddresses(struct rfc5444_writer *wr);
static void _cb_rreq_addMessageHeader(struct rfc5444_writer *wr, struct rfc5444_writer_message *message);

static void _cb_rrep_addMessageTLVs(struct rfc5444_writer *wr);
static void _cb_rrep_addAddresses(struct rfc5444_writer *wr);
static void _cb_rrep_addMessageHeader(struct rfc5444_writer *wr, struct rfc5444_writer_message *message);

static void _cb_addPacketHeader(struct rfc5444_writer *wr, struct rfc5444_writer_target *interface_1);

static uint8_t _msg_buffer[128];
static uint8_t _msg_addrtlvs[1000];
static uint8_t _packet_buffer[128];

static struct rfc5444_writer_message *_rreq_msg;
static struct rfc5444_writer_message *_rrep_msg;

uint8_t _msg_seqno;
uint16_t _pkt_seqno;


/**
 * Callback to define the packet header for a RFC5444 packet. This is actually
 * quite useless because a RREP/RREQ packet header should have no flags set, but anyway,
 * here's how you do it in case you need to.
 */
static void
_cb_addPacketHeader(struct rfc5444_writer *wr, struct rfc5444_writer_target *interface_1)
{
    printf("[aodvv2] %s()\n", __func__);

    /* set header with sequence number */
    rfc5444_writer_set_pkt_header(wr, interface_1, true);

    /* set sequence number */
    rfc5444_writer_set_pkt_seqno(wr, interface_1, _pkt_seqno);
}

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
    { .type = RFC5444_MSGTLV_SEQNUM },
    { .type = RFC5444_MSGTLV_METRIC },
};

/**
 * Callback to define the message header for a RFC5444 RREQ message
 * @param wr
 * @param message
 */
static void
_cb_rreq_addMessageHeader(struct rfc5444_writer *wr, struct rfc5444_writer_message *message)
{
    printf("[aodvv2] %s()\n", __func__);

    /* no originator, no hopcount, has hoplimit, no seqno */
    rfc5444_writer_set_msg_header(wr, message, false, false, true, false);
    rfc5444_writer_set_msg_hoplimit(wr, message, AODVV2_MAX_HOPCOUNT);
}

/**
 * Callback to add addresses and address TLVs to a RFC5444 RREQ message
 * @param wr
 */
static void
_cb_rreq_addAddresses(struct rfc5444_writer *wr)
{
    printf("[aodvv2] %s()\n", __func__);

    struct rfc5444_writer_address *origNode_addr, *targNode_addr;
    struct netaddr na_origNode, na_targNode;

    /* make sure we don't mess up the SeqNum */
    mutex_lock(&m_seqnum);
    int origNode_seqNum = get_seqNum();
    mutex_unlock(&m_seqnum);
    
    int origNode_hopCt = 9;

    if (netaddr_from_string(&na_origNode, "::1")) {
        return;
    }
    if (netaddr_from_string(&na_targNode, "::42")) {
        return;
    }

    /* add origNode address (has no address tlv); is mandatory address */
    origNode_addr = rfc5444_writer_add_address(wr, _rreq_message_content_provider.creator, &na_origNode, true);

    /* add origNode address (has no address tlv); is mandatory address */
    targNode_addr = rfc5444_writer_add_address(wr, _rreq_message_content_provider.creator, &na_targNode, true);

    /* Add Address TLVs to both addresses, effectively turning it into an
       AddressBlockTLV. */

    // add SeqNum TLVs
    // TODO: allow_dup true or false?
    rfc5444_writer_add_addrtlv(wr, origNode_addr, &_rreq_addrtlvs[0], &origNode_seqNum, sizeof(origNode_seqNum), false  );
    rfc5444_writer_add_addrtlv(wr, targNode_addr, &_rreq_addrtlvs[0], &origNode_seqNum, sizeof(origNode_seqNum), false  );

    // add Metric TLVs
    rfc5444_writer_add_addrtlv(wr, origNode_addr, &_rreq_addrtlvs[1], &origNode_hopCt, sizeof(origNode_hopCt), false);
    rfc5444_writer_add_addrtlv(wr, targNode_addr, &_rreq_addrtlvs[1], &origNode_hopCt, sizeof(origNode_hopCt), false );
}

/*
 * message content provider that will add message TLVs,
 * addresses and address block TLVs to all messages of type RREQ.
 */
static struct rfc5444_writer_content_provider _rrep_message_content_provider = {
    .msg_type = RFC5444_MSGTYPE_RREP,
    .addAddresses = _cb_rrep_addAddresses,
};

/* declaration of all address TLVs added to the RREQ message */
static struct rfc5444_writer_tlvtype _rrep_addrtlvs[] = {
    { .type = RFC5444_MSGTLV_SEQNUM },
    { .type = RFC5444_MSGTLV_METRIC },
};

/**
 * Callback to define the message header for a RFC5444 RREQ message
 * @param wr
 * @param message
 */
static void
_cb_rrep_addMessageHeader(struct rfc5444_writer *wr, struct rfc5444_writer_message *message)
{
    printf("[aodvv2] %s()\n", __func__);

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
    printf("[aodvv2] %s()\n", __func__);

    //struct rfc5444_writer_address *addr;
    struct rfc5444_writer_address *origNode_addr, *targNode_addr;
    struct netaddr na_origNode, na_targNode;

    int origNode_seqNum = 0;
    
    /* make sure we don't mess up the SeqNum */
    mutex_lock(&m_seqnum);
    int targNode_seqNum = get_seqNum();
    mutex_unlock(&m_seqnum);

    int targNode_hopCt = 1;

    if (netaddr_from_string(&na_origNode, "::42")) {
        return;
    }
    if (netaddr_from_string(&na_targNode, "::1")) {
        return;
    }

    /* add origNode address (has no address tlv); is mandatory address */
    origNode_addr = rfc5444_writer_add_address(wr, _rrep_message_content_provider.creator, &na_origNode, true);

    /* add origNode address (has no address tlv); is mandatory address */
    targNode_addr = rfc5444_writer_add_address(wr, _rrep_message_content_provider.creator, &na_targNode, true);

   /* Add Address TLVs to both addresses, effectively turning it into an
       AddressBlockTLV. */

    // add OrigNode and TargNode SeqNum TLVs
    // TODO: allow_dup true or false?
    rfc5444_writer_add_addrtlv(wr, origNode_addr, &_rrep_addrtlvs[0], &origNode_seqNum, sizeof(origNode_seqNum), false  );
    rfc5444_writer_add_addrtlv(wr, targNode_addr, &_rrep_addrtlvs[0], &origNode_seqNum, sizeof(origNode_seqNum), false  );

    rfc5444_writer_add_addrtlv(wr, origNode_addr, &_rrep_addrtlvs[0], &targNode_seqNum, sizeof(targNode_seqNum), false  );
    rfc5444_writer_add_addrtlv(wr, targNode_addr, &_rreq_addrtlvs[0], &targNode_seqNum, sizeof(targNode_seqNum), false  );

    // add Metric TLVs
    rfc5444_writer_add_addrtlv(wr, origNode_addr, &_rrep_addrtlvs[1], &targNode_hopCt, sizeof(targNode_hopCt), false);
    rfc5444_writer_add_addrtlv(wr, targNode_addr, &_rrep_addrtlvs[1], &targNode_hopCt, sizeof(targNode_hopCt), false );
}

void writer_init(write_packet_func_ptr ptr)
{
    printf("[aodvv2] %s()\n", __func__);

    /* set dummy values */
    _pkt_seqno = 7;
    _msg_seqno = 13;

    /* define interface for generating rfc5444 packets */
    interface_1.packet_buffer = _packet_buffer;
    interface_1.packet_size = sizeof(_packet_buffer);
    interface_1.addPacketHeader = _cb_addPacketHeader;
    /* set function to send binary packet content */
    interface_1.sendPacket = ptr;
    
    /* define the rfc5444 writer */
    writer.msg_buffer = _msg_buffer;
    writer.msg_size = sizeof(_msg_buffer);
    writer.addrtlv_buffer = _msg_addrtlvs;
    writer.addrtlv_size = sizeof(_msg_addrtlvs);

    /* initialize writer */
    rfc5444_writer_init(&writer);

    /* register a target (for sending messages to) in writer */
    rfc5444_writer_register_target(&writer, &interface_1);

    /* register a message content providers for RREQ and RREP */
    rfc5444_writer_register_msgcontentprovider(&writer, &_rreq_message_content_provider, _rreq_addrtlvs, ARRAYSIZE(_rreq_addrtlvs));
    rfc5444_writer_register_msgcontentprovider(&writer, &_rrep_message_content_provider, _rrep_addrtlvs, ARRAYSIZE(_rrep_addrtlvs));

    /* register rreq and rrep messages with 16 byte (ipv6) addresses.
       AddPacketHeader & addMessageHeader callbacks are triggered here. */
    _rreq_msg = rfc5444_writer_register_message(&writer, RFC5444_MSGTYPE_RREQ, false, RFC5444_MAX_ADDRLEN);
    _rrep_msg = rfc5444_writer_register_message(&writer, RFC5444_MSGTYPE_RREP, false, RFC5444_MAX_ADDRLEN);

    _rreq_msg->addMessageHeader = _cb_rreq_addMessageHeader;
    _rrep_msg->addMessageHeader = _cb_rrep_addMessageHeader;
}

void writer_send_rreq(void){

    DEBUG("[RREQ]\n");

    rfc5444_writer_create_message_alltarget(&writer, RFC5444_MSGTYPE_RREQ);
    rfc5444_writer_flush(&writer, &interface_1, false);
}

void writer_send_rrep(void){

    DEBUG("[RREP]\n");

    rfc5444_writer_create_message_alltarget(&writer, RFC5444_MSGTYPE_RREP);
    rfc5444_writer_flush(&writer, &interface_1, false);
}

void writer_cleanup(void)
{
    printf("[aodvv2] %s()\n", __func__);
}