#include <stdio.h>

#include "seqnum.h"
#include "cunit/cunit.h"

static void test_seqnum_basic_functionality(void)
{
    uint16_t seqNum;
    seqNum_init();

    START_TEST();
    seqNum = seqNum_get();
    CHECK_TRUE(seqNum == 1, "SeqNum should be 1 instead of %d\n", seqNum);
    seqNum_inc();
    seqNum = seqNum_get();
    CHECK_TRUE(seqNum == 2, "SeqNum should be 2 instead of %d\n", seqNum);    
    END_TEST();
}

/* Check if comparison works correctly around tipping point from 65535 to 1 */    
static void test_seqnum_comparison_around_tipping_point(void)
{
    uint16_t seqNum;
    int seqNum_cmp;
    seqNum_init();

    while (seqNum_get() < 65525){
        seqNum_inc();
    }

    START_TEST();
    seqNum = seqNum_get();
    CHECK_TRUE(seqNum_get() == 65525, "SeqNum should be 65525 instead of %d\n", seqNum);
    seqNum = seqNum_get();
    seqNum_cmp = seqnum_cmp(seqNum, 1);
    CHECK_TRUE(seqNum_cmp == -1, "SeqNum should be smaller than 1 instead of cmp_seqnum result %i\n", seqnum_cmp);    
    END_TEST();
}

/* Check if SeqNum tips over from 65535 to 1 correctly */
static void test_seqnum_tipping()
{
    uint16_t seqNum;
    seqNum_init();

    while (seqNum_get() < 65535){
        seqNum_inc();
    }

    START_TEST();
    seqNum = seqNum_get();
    CHECK_TRUE(seqNum == 65535, "SeqNum should be 655235 instead of %d\n", seqNum);    
    seqNum_inc();
    seqNum = seqNum_get();
    CHECK_TRUE(seqNum == 1, "SeqNum should be 1 instead of %d\n", seqNum);    
    END_TEST();
}


void test_seqnum_main(void)
{
    BEGIN_TESTING(NULL);

    test_seqnum_tipping();
    test_seqnum_comparison_around_tipping_point();
    test_seqnum_basic_functionality();

    FINISH_TESTING();
}
