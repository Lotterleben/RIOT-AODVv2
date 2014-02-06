#include "seqnum.h"

#define ENABLE_DEBUG (1)
#include "debug.h"

static uint16_t seqNum; 

/**
 * Initializes the AODVv2 SeqNum.
 **/

void seqNum_init(void)
{
    seqNum = 1;
}

/**
 * Increments the AODVv2 SeqNum by 1.
 **/
void seqNum_inc(void)
{   
    if (seqNum == 65535)
        seqNum = 1;
    else if (seqNum == 0)
        DEBUG("ERROR: SeqNum shouldn't be 0! \n"); // TODO handle properly
    else
        seqNum++;
}

uint16_t seqNum_get(void)
{
    return seqNum;
}

/**
 * @return -1 when s1 is smaller, 0 if equal, 1 if s1 is bigger.
 */
int seqnum_cmp(uint16_t s1, uint16_t s2)
{
    uint16_t diff = s1 - s2;
    printf("\t diff is: %i\n");
    if (diff == 0)
        return 0;
    if ((0 < diff) && (diff < 32768))
        return 1;
    else if (32768 <= diff <= 65535)
        return -1;
}
