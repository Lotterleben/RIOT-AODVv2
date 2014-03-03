#ifndef AODV_H_
#define AODV_H_

#include <sixlowpan/ip.h>
#include "sixlowpan.h"
#include "kernel.h"
#include "destiny.h"
#include "destiny/socket.h"
#include "transceiver.h"
#include "net_help.h"
#include "mutex.h"

#include "constants.h"
#include "seqnum.h"
#include "routing.h" 
#include "utils.h"
#include "reader.h"
#include "writer.h"
#include "thread.h"

struct rreq_rrep_data {
    struct aodvv2_packet_data* packet_data;
    struct netaddr* next_hop;
};
 
struct rerr_data {
    struct unreachable_node* unreachable_nodes; // Beware, this is the start of an array.
    int len;
    int hoplimit;
    struct netaddr* next_hop;
};

struct msg_container {
    int type;
    void* data;  
};

void aodv_set_metric_type(int metric_type);
static ipv6_addr_t* aodv_get_next_hop(ipv6_addr_t* dest);

void aodv_send_rreq(struct aodvv2_packet_data* packet_data);
void aodv_send_rrep(struct aodvv2_packet_data* packet_data, struct netaddr* next_hop);
void aodv_send_rerr(struct unreachable_node unreachable_nodes[], int len, int hoplimit, struct netaddr* next_hop);

#endif /* AODV_H_ */
