#include <mutex.h>

mutex_t m_seqnum; // TODO: find name that makes more sense
int test;

void aodv_init(void);
uint16_t get_seqNum(void);
void inc_seqNum(void);
void receive_udp(char *str);
void send_rreq(char *str);
void send_rrep(char *str);
