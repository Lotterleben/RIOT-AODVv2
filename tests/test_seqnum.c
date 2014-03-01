#include <stdio.h>

#include "seqnum.h"
#include "cunit/cunit.h"

static void test_seqnum_basic_functionality(void)
{
    uint16_t seqnum;
    seqnum_init();

    START_TEST();
    seqnum = seqnum_get();
    CHECK_TRUE(seqnum == 1, "seqnum should be 1 instead of %d\n", seqnum);
    seqnum_inc();
    seqnum = seqnum_get();
    CHECK_TRUE(seqnum == 2, "seqnum should be 2 instead of %d\n", seqnum);    
    END_TEST();
}

/* Check if comparison works correctly around tipping point from 65535 to 1 */    
static void test_seqnum_comparison_around_tipping_point(void)
{
    uint16_t seqnum;
    int seqnum_c;
    seqnum_init();

    while (seqnum_get() < 65525){
        seqnum_inc();
    }

    START_TEST();
    seqnum = seqnum_get();
    CHECK_TRUE(seqnum_get() == 65525, "seqnum should be 65525 instead of %d\n", seqnum);
    seqnum = seqnum_get();
    seqnum_c = seqnum_cmp(seqnum, 1);
    CHECK_TRUE(seqnum_c == -1, "seqnum should be smaller than 1 instead of cmp_seqnum result %i\n", seqnum_c);    
    END_TEST();
}

/* Check if seqnum tips over from 65535 to 1 correctly */
static void test_seqnum_tipping()
{
    uint16_t seqnum;
    seqnum_init();

    while (seqnum_get() < 65535){
        seqnum_inc();
    }

    START_TEST();
    seqnum = seqnum_get();
    CHECK_TRUE(seqnum == 65535, "seqnum should be 655235 instead of %d\n", seqnum);    
    seqnum_inc();
    seqnum = seqnum_get();
    CHECK_TRUE(seqnum == 1, "seqnum should be 1 instead of %d\n", seqnum);    
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
