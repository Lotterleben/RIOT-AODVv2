#ifndef READER_H_
#define READER_H_
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include "common/common_types.h"
#include "common/netaddr.h"
#include "rfc5444/rfc5444_reader.h"
#include "mutex.h"

#include "utils.h"
#include "routing.h"
#include "constants.h"
#include "seqnum.h"
#include "aodv.h"

void reader_init(void);
void reader_cleanup(void);
int reader_handle_packet(void* buffer, size_t length, struct netaddr* sender);
#endif /* READER_H_ */
