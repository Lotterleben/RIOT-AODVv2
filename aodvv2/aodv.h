#include <sixlowpan/ip.h>
#include "sixlowpan.h"
#include "kernel.h"
#include "destiny.h"
#include "destiny/socket.h"
#include "transceiver.h"
#include "net_help.h"
#include "mutex.h"

#include "include/aodvv2.h"
#include "seqnum.h"
#include "routing.h" 
#include "utils.h"
#include "reader.h"
#include "writer.h"
#include "thread.h"

#ifndef AODV_H_
#define AODV_H_

void aodv_set_metric_type(int metric_type);
static ipv6_addr_t* aodv_get_next_hop(ipv6_addr_t* dest);

#endif /* AODV_H_ */
