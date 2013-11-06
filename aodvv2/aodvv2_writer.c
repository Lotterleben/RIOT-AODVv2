#include <string.h>
#include <stdio.h>
#ifdef RIOT
#include "net_help.h"
#endif
#include "common/common_types.h"
#include "common/netaddr.h"
#include "rfc5444/rfc5444_writer.h"

#include "aodvv2/aodvv2_writer.h"

#include "include/aodvv2.h"

/**
 * Writer to create aodvv2 RFC5444 RREQ and RREP messages.
 * Please note that this is work under construction, specifically:
 * - The messageTLVs will be changed into Address TLVs or Address Block TLVs as
 *   soon as Charlie has answered my questions on the subject
 * - The Packet header data is bullshit and only serves as an example for me 
 **/

static void _cb_rreq_addMessageTLVs(struct rfc5444_writer *wr);
static void _cb_rrep_addMessageTLVs(struct rfc5444_writer *wr);

static void _cb_rreq_addAddresses(struct rfc5444_writer *wr);
static void _cb_rrep_addAddresses(struct rfc5444_writer *wr);

static void _cb_addPacketHeader(struct rfc5444_writer *wr, struct rfc5444_writer_target *interface_1);

static uint8_t _msg_buffer[128];
static uint8_t _msg_addrtlvs[1000];
static uint8_t _packet_buffer[128];

static struct rfc5444_writer_message *_rrep_msg;
static struct rfc5444_writer_message *_rreq_msg;


uint8_t _msg_seqno;
uint16_t _pkt_seqno;

/* define interface for generating rfc5444 packets */
struct rfc5444_writer_target interface_1 = {
    .packet_buffer = _packet_buffer,
    .packet_size = sizeof(_packet_buffer),
    .addPacketHeader = _cb_addPacketHeader
};

/* define a rfc5444 writer */
struct rfc5444_writer writer = {
    .msg_buffer = _msg_buffer,
    .msg_size = sizeof(_msg_buffer),
    .addrtlv_buffer = _msg_addrtlvs,
    .addrtlv_size = sizeof(_msg_addrtlvs),
};

/*
 * message content provider that will add message TLVs,
 * addresses and address block TLVs to all messages of type RREQ.
 */
static struct rfc5444_writer_content_provider _rreq_message_content_provider = {
    .msg_type = RREQ,
    .addMessageTLVs = _cb_rreq_addMessageTLVs,
    .addAddresses = _cb_rreq_addAddresses,
};

/*
 * message content provider that will add message TLVs,
 * addresses and address block TLVs to all messages of type RREP.
 */
static struct rfc5444_writer_content_provider _rrep_message_content_provider = {
    .msg_type = RREP,
    .addMessageTLVs = _cb_rrep_addMessageTLVs,
    .addAddresses = _cb_rrep_addAddresses,
};

/* declaration of all address TLVs added to the RREQ message */
static struct rfc5444_writer_tlvtype _rreq_addrtlvs[] = {
    { .type = RFC5444_MSGTLV_SEQNUM },
    { .type = RFC5444_MSGTLV_METRIC },
};

/* declaration of all address TLVs added to the RREP message */

/* TODO: add correct tlvs */
static struct rfc5444_writer_tlvtype _rrep_addrtlvs[] = {
    { .type = RFC5444_MSGTLV_SEQNUM },
    { .type = RFC5444_MSGTLV_METRIC },
};

/**
 * Callback to define the packet header for a RFC5444 packet. This is actually
 * quite useless because a RREQ packet header should have no flags set, but anyway,
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

/**
 * Callback to define the message header for a RFC5444 RREQ message
 * @param wr
 * @param message
 */
static void
_cb_rreq_addMessageHeader(struct rfc5444_writer *wr, struct rfc5444_writer_message *message)
{
    printf("[aodvv2] %s()\n", __func__);

    /* no originator, no hopcunt, has hoplimit, no seqno */
    rfc5444_writer_set_msg_header(wr, message, false, false, true, false);

    /* set seqno */
    rfc5444_writer_set_msg_seqno(wr, message, _msg_seqno);
}

/**
 * Callback to define the message header for a RFC5444 RREP message
 * @param wr
 * @param message
 */
static void
_cb_rrep_addMessageHeader(struct rfc5444_writer *wr, struct rfc5444_writer_message *message)
{
    printf("[aodvv2] %s()\n", __func__);

    /* no originator, no hopcunt, has hoplimit, no seqno */
    rfc5444_writer_set_msg_header(wr, message, false, false, true, false);

    /* set seqno */
    rfc5444_writer_set_msg_seqno(wr, message, _msg_seqno);
}

/**
 * Callback to add message TLVs to a RFC5444 RREQ message.
 * @param wr
 */
static void
_cb_rreq_addMessageTLVs(struct rfc5444_writer *wr)
{
    printf("[aodvv2] %s()\n", __func__);
    int _rreq_foo;

    /* add message tlv type SeqNum (exttype 0) with 4-byte value 23 */
    _rreq_foo = htonl(23);
    rfc5444_writer_add_messagetlv(wr, RFC5444_MSGTLV_SEQNUM, 0, &_rreq_foo, sizeof (_rreq_foo));

    /* add message tlv type Metric (ext 0) with 4-byte value 42 */
    _rreq_foo = htonl(42);
    rfc5444_writer_add_messagetlv(wr, RFC5444_MSGTLV_METRIC, 0, &_rreq_foo, sizeof (_rreq_foo));
}


/**
 * Callback to add message TLVs to a RFC5444 RREP message.
 * @param wr
 */
static void
_cb_rrep_addMessageTLVs(struct rfc5444_writer *wr)
{
    int _rrep_foo;
    printf("[aodvv2] %s()\n", __func__);

    /* add message tlv type OrignNode SeqNum (exttype 0) with 4-byte value 8 */
    _rrep_foo = htonl(8);
    rfc5444_writer_add_messagetlv(wr, RFC5444_MSGTLV_ORIGNODE_SEQNUM, 0, &_rrep_foo, sizeof (_rrep_foo));

    /* add message tlv type OrignNode SeqNum (exttype 0) with 4-byte value 8 */
    _rrep_foo = htonl(8);
    rfc5444_writer_add_messagetlv(wr, RFC5444_MSGTLV_TARGNODE_SEQNUM, 0, &_rrep_foo, sizeof (_rrep_foo));

    /* add message tlv type Metric (ext 0) with 4-byte value 42 */
    _rrep_foo = htonl(90);
    rfc5444_writer_add_messagetlv(wr, RFC5444_MSGTLV_METRIC, 0, &_rrep_foo, sizeof (_rrep_foo));
}

/**
 * Callback to add addresses and address TLVs to a RFC5444 RREQ message
 * @param wr
 */
static void
_cb_rreq_addAddresses(struct rfc5444_writer *wr)
{
    printf("[aodvv2] %s()\n", __func__);

    struct rfc5444_writer_address *addr;
    struct netaddr na_origNode, na_targNode;

    if (netaddr_from_string(&na_origNode, "127.0.0.1")) {
    return;
    }
    if (netaddr_from_string(&na_targNode, "127.0.0.42")) {
    return;
    }

    /* add origNode address (has no address tlv); is mandatory address */
    rfc5444_writer_add_address(wr, _rreq_message_content_provider.creator, &na_origNode, true);

    /* add origNode address (has no address tlv); is mandatory address */
    rfc5444_writer_add_address(wr, _rreq_message_content_provider.creator, &na_targNode, true);
}

/**
 * Callback to add addresses and address TLVs to a RFC5444 RREP message
 * @param wr
 */
static void
_cb_rrep_addAddresses(struct rfc5444_writer *wr)
{
    printf("[aodvv2] %s()\n", __func__);
    /* TODO */
}

void writer_init(write_packet_func_ptr ptr)
{
    printf("[aodvv2] %s()\n", __func__);

    struct rfc5444_writer_message *_rreq_msg;

    /* set dummy values */
    _pkt_seqno = 7;
    _msg_seqno = 13;

    /* initialize writer */
    rfc5444_writer_init(&writer);

    /* register a target (for sending messages to) in writer */
    rfc5444_writer_register_target(&writer, &interface_1);

    /* register a message content providers for RREQ and RREP */
    rfc5444_writer_register_msgcontentprovider(&writer, &_rreq_message_content_provider, _rreq_addrtlvs, ARRAYSIZE(_rreq_addrtlvs));
    rfc5444_writer_register_msgcontentprovider(&writer, &_rrep_message_content_provider, _rrep_addrtlvs, ARRAYSIZE(_rrep_addrtlvs));

    /* register rreq and rrep messages with 4 byte addresses */
    _rreq_msg = rfc5444_writer_register_message(&writer, RREQ, false, 4);
    _rrep_msg = rfc5444_writer_register_message(&writer, RREQ, false, 4);

    _rreq_msg->addMessageHeader = _cb_rreq_addMessageHeader;
    //_rrep_msg->addMessageHeader = _cb_rrep_addMessageHeader; /* segfault. */

    /* set function to send binary packet content */
    interface_1.sendPacket = ptr;
}

void writer_cleanup(void)
{
    printf("[aodvv2] %s()\n", __func__);
}