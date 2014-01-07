#include <mutex.h>

mutex_t m_seqnum; // TODO: find name that makes more sense
int test;

void sender_init(void);
void init_seqNum(void);
uint16_t get_seqNum(void);
void inc_seqNum(void);
int cmp_seqnum(uint32_t s1, uint32_t s2);
void receive_udp(char *str);
void send_rreq(char *str);
void send_rrep(char *str);
