#ifndef AODVV2_H_
#define AODVV2_H_

#define AODVV2_MAX_HOPCOUNT 20
#define AODVV2_MAX_ROUTING_ENTRIES 255
#define AODVV2_DEFAULT_METRIC_TYPE 3
#define AODVV2_ACTIVE_INTERVAL 5 // seconds
#define AODVV2_MAX_IDLETIME 200  // seconds // TODO: change back to proper value

/* RFC5498 */
#define MANET_PORT  269

// I'M SO SORRY
#define MY_IP "::42"

enum msg_type {
    RFC5444_MSGTYPE_RREQ,
    RFC5444_MSGTYPE_RREP,
};

enum tlv_type {
    //RFC5444_MSGTLV_SEQNUM, // TODO: auskommentieren?
    RFC5444_MSGTLV_ORIGSEQNUM,
    RFC5444_MSGTLV_TARGSEQNUM,    
    RFC5444_MSGTLV_METRIC,
};
#endif /* AODVV2_H_ */
