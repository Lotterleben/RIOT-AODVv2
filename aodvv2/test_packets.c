#include <stdio.h>

#include "include/aodvv2.h"

#include "cunit/cunit.h"
#include "common/common_types.h"
#include "common/netaddr.h"
#include "rfc5444/rfc5444_context.h"

#define DISALLOW_CONSUMER_CONTEXT_DROP false

struct netaddr addr;

static void
test_read_write_drop_packet(struct rfc5444_writer *wr __attribute__ ((unused)),
        struct rfc5444_writer_target *iface __attribute__((unused)),
        void *buffer, size_t length)
{
    int result = reader_handle_packet(buffer, length, &addr);
    CHECK_TRUE(result == RFC5444_DROP_PACKET, "Packet should've been dropped\n");    
}

static void
test_read_write_redundant_packet(struct rfc5444_writer *wr __attribute__ ((unused)),
        struct rfc5444_writer_target *iface __attribute__((unused)),
        void *buffer, size_t length)
{
    reader_handle_packet(buffer, length, &addr);
    int result = reader_handle_packet(buffer, length, &addr);
    printf("res: %i\n", result);
    CHECK_TRUE(result == RFC5444_DROP_PACKET, "Packet should've been dropped\n");    
}

static void test_seqnum_basic_functionality(void)
{
    uint16_t seqNum;
    seqNum = get_seqNum();
    CHECK_TRUE(seqNum == 1, "SeqNum should be 1 instead of %d\n", seqNum);
    inc_seqNum();
    seqNum = get_seqNum();
    CHECK_TRUE(seqNum == 2, "SeqNum should be 2 instead of %d\n", seqNum);    
}

/* Check if comparison works correctly around tipping point from 65535 to 1 */    
static void test_seqnum_comparison_around_tipping_point(void)
{
    uint16_t seqNum;
    int seqNum_cmp;

    while (get_seqNum() < 65525){
        inc_seqNum();
    }
    seqNum = get_seqNum();
    CHECK_TRUE(get_seqNum() == 65525, "SeqNum should be 65525 instead of %d\n", seqNum);
    seqNum = get_seqNum();
    seqNum_cmp = cmp_seqnum(seqNum, 1);
    CHECK_TRUE(seqNum_cmp == -1, "SeqNum should be smaller than 1 instead of cmp_seqnum result %i\n", cmp_seqnum);    
}

/* Check if SeqNum tips over from 65535 to 1 correctly */
static void test_seqnum_tipping()
{
    uint16_t seqNum;

    while (get_seqNum() < 65535){
        inc_seqNum();
    }
    seqNum = get_seqNum();
    CHECK_TRUE(seqNum == 65535, "SeqNum should be 655235 instead of %d\n", seqNum);    
    inc_seqNum();
    seqNum = get_seqNum();
    CHECK_TRUE(seqNum == 1, "SeqNum should be 1 instead of %d\n", seqNum);    

}

void test_rreq_dropped(void)
{
    static struct netaddr origNode, targNode;

    netaddr_from_string(&origNode, MY_IP);
    netaddr_from_string(&targNode, "::13");

    /* init reader and writer */
    reader_init();
    writer_init(test_read_write_drop_packet);
    
    START_TEST();

    writer_send_rreq(NULL, &targNode); // right now, this won't lead to test_read_write_packet_dropped being called
    /* fuck, kann ich dem reader iwie adressen ohne OrigNode/TargNode Adresse, metric,
     hoplimit oder OrigNode / TargNode SeqNum füttern ohne im writer rumzupfuschen
     oder noch nen extrawriter zu schreiben?  wie fake ich überhaupt daten? 
     ich glaub ich muss die pakete bauen, buffer auslesen und speichern.. yikes*/ 

    writer_cleanup();
    /* init writer again */
    writer_init(test_read_write_redundant_packet);
    writer_send_rreq(&origNode, &targNode);

    END_TEST();

    /* cleanup */
    reader_cleanup();
    writer_cleanup();
}

void test_seqNum(void){
    init_seqNum();
    uint16_t seqNum;
    int seqNum_cmp, test;

    START_TEST();

    test_seqnum_basic_functionality();
    test_seqnum_comparison_around_tipping_point();
    test_seqnum_tipping();  

    END_TEST();
}


//int main(int argc __attribute__ ((unused)), char **argv __attribute__ ((unused))) {
void test_packets_main(void)
{
    netaddr_from_string(&addr, "::111");
    
    BEGIN_TESTING(NULL);

    //test_rreq_dropped();
    test_seqNum();

    FINISH_TESTING();
}