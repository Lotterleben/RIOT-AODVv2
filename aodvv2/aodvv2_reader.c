#include <string.h>
#include <stdio.h>
#ifdef RIOT
#include "net_help.h"
#endif
#include "common/common_types.h"
#include "common/netaddr.h"
#include "rfc5444/rfc5444_reader.h"

#include "aodvv2_reader.h"
#include "include/aodvv2.h"

static struct rfc5444_reader reader;

static enum rfc5444_result _cb_rreq_blocktlv_messagetlvs_okay(
    struct rfc5444_reader_tlvblock_context *cont);
static enum rfc5444_result _cb_rreq_blocktlv_addresstlvs_okay(
    struct rfc5444_reader_tlvblock_context *cont);

static enum rfc5444_result _cb_rrep_blocktlv_messagetlvs_okay(
    struct rfc5444_reader_tlvblock_context *cont);
static enum rfc5444_result _cb_rrep_blocktlv_addresstlvs_okay(
    struct rfc5444_reader_tlvblock_context *cont);


/*
 * Message consumer, will be called once for every message of
 * type RFC5444_MSGTYPE_RREQ that contains all the mandatory message TLVs
 */
static struct rfc5444_reader_tlvblock_consumer _rreq_consumer = {
    /* parse message type 1 */
    .msg_id = RFC5444_MSGTYPE_RREQ,

    /* use a block callback */
    .block_callback = _cb_rreq_blocktlv_messagetlvs_okay,
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
 * Address consumer entries definition
 * TLV types RFC5444_MSGTLV_SEQNUM and RFC5444_MSGTLV_METRIC
 */
static struct rfc5444_reader_tlvblock_consumer_entry _rreq_address_consumer_entries[] = {
    [RFC5444_MSGTLV_ORIGNODE_SEQNUM] = { .type = RFC5444_MSGTLV_ORIGNODE_SEQNUM },
    [RFC5444_MSGTLV_METRIC] = { .type = RFC5444_MSGTLV_METRIC }
};

/*
 * Message consumer, will be called once for every message of
 * type RFC5444_MSGTYPE_RREP that contains all the mandatory message TLVs
 */
static struct rfc5444_reader_tlvblock_consumer _rrep_consumer = {
    /* parse message type 1 */
    .msg_id = RFC5444_MSGTYPE_RREP,

    /* use a block callback */
    .block_callback = _cb_rrep_blocktlv_messagetlvs_okay,
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
 * TLV types RFC5444_MSGTLV_ORIGNODE_SEQNUM, RFC5444_MSGTLV_TARGNODE_SEQNUM
 * and RFC5444_MSGTLV_METRIC
 */
static struct rfc5444_reader_tlvblock_consumer_entry _rrep_address_consumer_entries[] = {
    [RFC5444_MSGTLV_ORIGNODE_SEQNUM] = { .type = RFC5444_MSGTLV_ORIGNODE_SEQNUM },
    [RFC5444_MSGTLV_TARGNODE_SEQNUM] = { .type = RFC5444_MSGTLV_TARGNODE_SEQNUM },
    [RFC5444_MSGTLV_METRIC] = { .type = RFC5444_MSGTLV_METRIC }
};

/**
 * This block callback is called for every address
 *
 * @param cont
 * @return
 */
static enum rfc5444_result _cb_rreq_blocktlv_messagetlvs_okay(struct rfc5444_reader_tlvblock_context *cont)
{
    printf("[aodvv2] %s()\n", __func__);

    if (cont->has_hoplimit) {
        printf("[aodvv2] %s() \t i can has hop limit: %d\n",__func__ , &cont->hoplimit);
    }
    return RFC5444_OKAY;
}

/**
 * This block callback is called for every address
 *
 * @param cont
 * @return
 */
static enum rfc5444_result _cb_rreq_blocktlv_addresstlvs_okay(struct rfc5444_reader_tlvblock_context *cont)
{
    struct netaddr_str nbuf;
    struct rfc5444_reader_tlvblock_entry* tlv;
    int value;

    printf("[aodvv2] %s()\n", __func__);
    printf("\tmessage type: %d\n", cont->type);
    printf("\taddr: %s\n", netaddr_to_string(&nbuf, &cont->addr));

    /* handle SeqNum TLV */
    tlv = _rreq_address_consumer_entries[RFC5444_MSGTLV_ORIGNODE_SEQNUM].tlv;
    while (tlv) {
        /* values of TLVs are not aligned well in memory, so we have to copy them */
        memcpy(&value, tlv->single_value, sizeof(value));
        printf("\ttlv RFC5444_MSGTLV_ORIGNODE_SEQNUM: %d\n", ntohl(value));
        tlv = tlv->next_entry;
    }

    /* handle Metric TLV */
    tlv = _rreq_address_consumer_entries[RFC5444_MSGTLV_METRIC].tlv;
    while (tlv) {
        /* values of TLVs are not aligned well in memory, so we have to copy them */
        memcpy(&value, tlv->single_value, sizeof(value));
        printf("\ttlv RFC5444_MSGTLV_METRIC hopCt: %d\n", ntohl(value));
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
static enum rfc5444_result _cb_rrep_blocktlv_messagetlvs_okay(struct rfc5444_reader_tlvblock_context *cont)
{
    printf("[aodvv2] %s()\n", __func__);

    if (cont->has_hoplimit) {
        printf("[aodvv2] %s() \t i can has hop limit: %d\n",__func__ , &cont->hoplimit);
    }
    return RFC5444_OKAY;
}

/**
 * This block callback is called for every address
 *
 * @param cont
 * @return
 */
static enum rfc5444_result _cb_rrep_blocktlv_addresstlvs_okay(struct rfc5444_reader_tlvblock_context *cont)
{
    struct netaddr_str nbuf;
    struct rfc5444_reader_tlvblock_entry* tlv;
    int value;

    printf("[aodvv2] %s()\n", __func__);
    printf("\tmessage type: %d\n", cont->type);
    printf("\taddr: %s\n", netaddr_to_string(&nbuf, &cont->addr));

    /* handle OrigNode SeqNum TLV */
    tlv = _rrep_address_consumer_entries[RFC5444_MSGTLV_ORIGNODE_SEQNUM].tlv;
    while (tlv) {
        /* values of TLVs are not aligned well in memory, so we have to copy them */
        memcpy(&value, tlv->single_value, sizeof(value));
        printf("\ttlv RFC5444_MSGTLV_ORIGNODE_SEQNUM: %d\n", ntohl(value));
        tlv = tlv->next_entry;
    }

    /* handle TargNode SeqNum TLV */
    tlv = _rrep_address_consumer_entries[RFC5444_MSGTLV_TARGNODE_SEQNUM].tlv;
    while (tlv) {
        /* values of TLVs are not aligned well in memory, so we have to copy them */
        memcpy(&value, tlv->single_value, sizeof(value));
        printf("\ttlv RFC5444_MSGTLV_TARGNODE_SEQNUM: %d\n", ntohl(value));
        tlv = tlv->next_entry;
    }

    /* handle Metric TLV */
    tlv = _rrep_address_consumer_entries[RFC5444_MSGTLV_METRIC].tlv;
    while (tlv) {
        /* values of TLVs are not aligned well in memory, so we have to copy them */
        memcpy(&value, tlv->single_value, sizeof(value));
        printf("\ttlv RFC5444_MSGTLV_METRIC hopCt: %d\n", ntohl(value));
        tlv = tlv->next_entry;
    }
    return RFC5444_OKAY;
}

void reader_init(void)
{
    printf("%s()\n", __func__);

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
        _rreq_address_consumer_entries, ARRAYSIZE(_rreq_address_consumer_entries));
    rfc5444_reader_add_message_consumer(&reader, &_rrep_address_consumer,
        _rrep_address_consumer_entries, ARRAYSIZE(_rrep_address_consumer_entries));
}

void reader_cleanup(void)
{
    printf("%s() TODO\n", __func__);
    rfc5444_reader_cleanup(&reader);
}

int reader_handle_packet(void* buffer, size_t length) {
    return rfc5444_reader_handle_packet(&reader, buffer, length);
}
