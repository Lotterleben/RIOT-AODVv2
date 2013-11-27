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

/*
 * Message consumer, will be called once for every message with
 * type 1 that contains all the mandatory message TLVs
 */
static struct rfc5444_reader_tlvblock_consumer _rreq_consumer = {
  /* parse message type 1 */
  .msg_id = RFC5444_MSGTYPE_RREQ,

  /* use a block callback */
  .block_callback = _cb_rreq_blocktlv_messagetlvs_okay,
};

static enum rfc5444_result _cb_rreq_blocktlv_messagetlvs_okay(struct rfc5444_reader_tlvblock_context *cont)
{
    printf("[aodvv2] %s()\n", __func__);

    if (cont->has_hoplimit) {
        printf("[aodvv2] %s() \t i can has hop limit: %i\n",__func__ , &cont->hoplimit);
    }
}

void reader_init(void)
{
    printf("%s()\n", __func__);

    /* initialize reader */
    rfc5444_reader_init(&reader);

    /* register message consumer. We have no message TLVs, so we can leave the
     rfc5444_reader_tlvblock_consumer_entry empty */
    rfc5444_reader_add_message_consumer(&reader, &_rreq_consumer,
        NULL, 0);
}

void reader_cleanup(void)
{

}

int reader_handle_packet(void* buffer, size_t length) {
    return rfc5444_reader_handle_packet(&reader, buffer, length);
}
