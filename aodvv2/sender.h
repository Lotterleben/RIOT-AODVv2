#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "common/netaddr.h"

#include "rfc5444/rfc5444_reader.h"
#include "rfc5444/rfc5444_writer.h"
#include "rfc5444/rfc5444_print.h"

#include "destiny.h"
#include "destiny/socket.h"
#include "transceiver.h"
#include "net_help.h"

#include "include/aodvv2.h"
#include "aodvv2_reader.h"
#include "aodvv2_writer.h"
#include "seqnum.h"

#ifndef SENDER_H_
#define SENDER_H_

void sender_init(void);
void receive_udp(char *str);
void send_rreq(struct netaddr* origNode, struct netaddr* targNode);
void send_rrep(struct aodvv2_packet_data* packet_data, struct netaddr* next_hop);

#endif /* SENDER_H_ */