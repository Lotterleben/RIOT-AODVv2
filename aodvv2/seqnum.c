#include "seqnum.h"

#define ENABLE_DEBUG (1)
#include "debug.h"

static uint16_t seqNum; 

void init_seqNum(void)
{
    seqNum = 1;
}

/* NOTE: lock on m_seqnum before using this function! */

void inc_seqNum(void)
{   
    if (seqNum == 65535)
        seqNum = 1;
    else if (seqNum == 0)
        DEBUG("ERROR: SeqNum shouldn't be 0! \n"); // TODO handle properly
    else
        seqNum++;
}

/* NOTE: lock on m_seqnum before using this function! */
uint16_t get_seqNum(void)
{
    return seqNum;
}

/* 
 * returns -1 when s1 is smaller, 0 if equal, 1 if s1 is bigger.
 * reasoning:
    Suppose we have S1 and S2.
    **Naive Approach:**
    S1 < S2 : S2 is newer
    S1 > S2 : S1 is newer
    **but:**
    What if S1 = 65535 and S2 = 1? With our naive approach, S1 would be assumed 
    to be newer, which is clearly wrong.
    So we'll need some kind of "wiggle room" around the tipping point of 65535 and 1.
 */
int cmp_seqnum(uint32_t s1, uint32_t s2)
{
    int wiggle_room = 10;

    if (s1 < s2){
        if ((s1 <= wiggle_room) && (s2 >= 65535 - wiggle_room))
            return 1;
        return -1;
    }
    if (s1 > s2){
        if (s1 >= 65535 - wiggle_room && s2 <= wiggle_room)
            return -1;
        return 1;
    }
    return 0;
}
