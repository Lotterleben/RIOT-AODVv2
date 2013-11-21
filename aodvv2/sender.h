#include "destiny/socket.h"

#ifdef MODULE_NATIVENET
#define TRANSCEIVER TRANSCEIVER_NATIVE
#endif

void receive_udp(char *str);
void send_udp(char *str);
