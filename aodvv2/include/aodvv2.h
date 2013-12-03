/* beware, for these are dummy values */
#define AODVV2_MAX_HOPCOUNT 255
#define AODVV2_ROUTE_VALIDITY_TIME 10000

/* RFC5498 */
#define MANET_PORT  269

enum msg_type {
    RFC5444_MSGTYPE_RREQ,
    RFC5444_MSGTYPE_RREP,
};

enum tlv_type {
    RFC5444_MSGTLV_SEQNUM,
    RFC5444_MSGTLV_METRIC,
};
