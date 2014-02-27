#include "seqnum.h"

#define ENABLE_DEBUG (1)
#include "debug.h"

static uint16_t seqnum; 

/**
 * Initializes the AODVv2 SeqNum.
 **/

void seqnum_init(void)
{
    seqnum = 1;
}

/**
 * Increments the AODVv2 SeqNum by 1.
 **/
void seqnum_inc(void)
{   
    if (seqnum == 65535)
        seqnum = 1;
    else if (seqnum == 0)
        DEBUG("ERROR: SeqNum shouldn't be 0! \n"); // TODO handle properly
    else
        seqnum++;
}

uint16_t seqnum_get(void)
{
    return seqnum;
}

/**
 * @return -1 when s1 is smaller, 0 if equal, 1 if s1 is bigger.
 */
int seqnum_cmp(uint16_t s1, uint16_t s2)
{
    uint16_t diff = s1 - s2;
    if (diff == 0)
        return 0;
    if ((0 < diff) && (diff < 32768))
        return 1;
    //else if (32768 <= diff <= 65535)
    return -1;
}
