#ifndef WRITER_H_
#define WRITER_H_

#include "common/common_types.h"
#include "rfc5444/rfc5444_writer.h"
#include "reader.h"

#include "common/common_types.h"
#include "common/netaddr.h"
#include "rfc5444/rfc5444_writer.h"
#include "rfc5444/rfc5444_iana.h"
#include "mutex.h"

#include "constants.h"
#include "seqnum.h"

struct writer_target {
    struct rfc5444_writer_target interface;
    struct netaddr target_address;
    struct aodvv2_packet_data _packet_data;
};

typedef void (*write_packet_func_ptr)(
    struct rfc5444_writer *wr, struct rfc5444_writer_target *iface, void *buffer, size_t length);

void writer_init(write_packet_func_ptr ptr);
void writer_cleanup(void);

void writer_send_rreq(struct netaddr* na_origNode, struct netaddr* na_targNode, struct netaddr* next_hop);
void writer_forward_rreq(struct aodvv2_packet_data* packet_data, struct netaddr* next_hop);
void writer_send_rrep(struct aodvv2_packet_data* packet_data, struct netaddr* next_hop);

#endif /* WRITER_H_ */
