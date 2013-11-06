enum tlv_type {
    RFC5444_MSGTLV_SEQNUM, /* could be omitted & replaced with ORIGNODE_SEQNUM */
    RFC5444_MSGTLV_ORIGNODE_SEQNUM,
    RFC5444_MSGTLV_TARGNODE_SEQNUM,
    RFC5444_MSGTLV_METRIC,
};

/* For some reason message creation fails if msg_type is 0 */
enum msg_type {
    RREQ = 1,
    RREP = 2
};



// typedef struct socka6 sockaddr6_t;