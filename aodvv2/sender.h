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

#include "aodvv2_reader.h"
#include "aodvv2_writer.h"


void sender_init(void);
void init_seqNum(void);
uint16_t get_seqNum(void);
void inc_seqNum(void);
int cmp_seqnum(uint32_t s1, uint32_t s2);
void receive_udp(char *str);
void send_rreq(char *str);
void send_rrep(char *str);
