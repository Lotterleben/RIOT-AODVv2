#define AODVV2_MAX_HOPCOUNT 255

/* RFC5498 */
#define MANET_PORT  269

enum msg_type {
    RFC5444_MSGTYPE_RREQ,
    RFC5444_MSGTYPE_RREP,
};

enum tlv_type {
    RFC5444_MSGTLV_ORIGNODE_SEQNUM,
    RFC5444_MSGTLV_TARGNODE_SEQNUM,
    RFC5444_MSGTLV_METRIC,
};

/* for temporary storage so we can pass around pointers to handle the data we've
 received */

struct rreq_msg {
    uint8_t msg_hop_limit;
    struct netaddr origNode_addr;
};
