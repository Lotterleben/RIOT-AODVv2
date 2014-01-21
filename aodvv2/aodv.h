#include <sixlowpan/ip.h>
#include "sixlowpan.h"
#include "kernel.h"

#include "include/aodvv2.h"
#include "seqnum.h"
#include "routing.h" 
#include "utils.h"
#include "reader.h"
#include "writer.h"
#include "thread.h"

#ifndef AODV_H_
#define AODV_H_

void aodv_send(void);
static ipv6_addr_t* aodv_get_next_hop(ipv6_addr_t* dest);

#endif /* AODV_H_ */
