#ifndef SEQNUM_H_
#define SEQNUM_H_
#include <stdint.h>

void seqnum_init(void);
uint16_t seqnum_get(void);
void seqnum_inc(void);
int seqnum_cmp(uint16_t s1, uint16_t s2);

#endif /* SEQNUM_H_ */
