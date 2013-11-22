#include "destiny/socket.h"

#ifdef MODULE_NATIVENET
#define TRANSCEIVER TRANSCEIVER_NATIVE
#endif

void aodv_init(void);
void receive_udp(char *str);
void send_rreq(char *str);
void send_rrep(char *str);