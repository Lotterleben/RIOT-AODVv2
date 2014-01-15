#include <stdint.h>

#ifndef SEQNUM_H_
#define SEQNUM_H_

void init_seqNum(void);
uint16_t get_seqNum(void);
void inc_seqNum(void);
int cmp_seqnum(uint32_t s1, uint32_t s2);

#endif /* SEQNUM_H_ */