#include <stdio.h>

#include "include/aodvv2.h"
#include "utils.h"
#include "sender.h"

#include "cunit/cunit.h"
#include "common/common_types.h"
#include "common/netaddr.h"
#include "rfc5444/rfc5444_context.h"

static void test_write_packet(struct rfc5444_writer *wr __attribute__ ((unused)),
        struct rfc5444_writer_target *iface __attribute__((unused)),
        void *buffer, size_t length);
static void test_packets_init(void);
static void test_packets_cleanup(void);

static struct netaddr addr;
static struct autobuf _hexbuf;

/*
 * A lot of these are smoke tests; you'll have to look at the debug output to 
 * see if the reader/writer behave as exprected since there's no way of judging
 * from the outside :(
 */

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

static void test_rreq_dropped(void)
{
    static struct netaddr origNode, targNode;

    netaddr_from_string(&origNode, MY_IP);
    netaddr_from_string(&targNode, "::13");
   
    test_packets_init();

    START_TEST();
    printf("== Testing if no origNode data -> no RREQ: ==\n");
    writer_send_rreq(NULL, &targNode); // right now, this won't lead to test_read_write_packet_dropped being called
    /* fuck, kann ich dem reader iwie adressen ohne OrigNode/TargNode Adresse, metric,
     hoplimit oder OrigNode / TargNode SeqNum füttern ohne im writer rumzupfuschen
     oder noch nen extrawriter zu schreiben?  wie fake ich überhaupt daten? 
     ich glaub ich muss die pakete bauen, buffer auslesen und speichern.. yikes*/ 

    writer_cleanup();
    writer_init(test_write_packet);    

    writer_send_rreq(&origNode, &targNode);
    printf("== Testing if duplicate RREQ is dropped: ==\n");
    writer_send_rreq(&origNode, &targNode);

    END_TEST();
    test_packets_cleanup();
}

/* Test if a RREP that's addressed to one of my clients is turned into a RREP and sent */
static void test_rreq_to_rrep(void)
{
    static struct netaddr origNode, targNode;
    init_clienttable();
    
    netaddr_from_string(&targNode, MY_IP);
    netaddr_from_string(&origNode, "::14");

    add_client(&targNode, 128);
    START_TEST();
    printf("== Testing if RREQ to one of my client nodes is answered with a RREP: ==\n");
    send_rreq(&origNode, &targNode);
    END_TEST();
}


// TODO: testen ob's korrekt geforwardet wurde?
static void test_rrep(void)
{
    static struct netaddr address, origNode, targNode;
    
    netaddr_from_string(&address, "::12");
    netaddr_from_string(&targNode, "::13");
    netaddr_from_string(&origNode, "::14");

    struct aodvv2_packet_data entry_1 = {
        .hoplimit = AODVV2_MAX_HOPCOUNT,
        .sender = address,
        .metricType = AODVV2_DEFAULT_METRIC_TYPE,
        .origNode = {
            .addr = origNode,
            .prefixlen = 128,
            .metric = 12,
            .seqNum = 13,
        },
        .targNode = {
            .addr = targNode,
            .prefixlen = 128,
            .metric = 12,
            .seqNum = 0,
        },
        .timestamp = NULL,
    };

    test_packets_init();

    START_TEST();
    //TODO: fixme
    //printf("== Testing if RREP to someone else is forwarded: ==\n");
    //send_rrep(&entry_1, &targNode);
    printf("== Testing if RREP to me is not forwarded: ==\n");
    netaddr_from_string(&entry_1.origNode.addr, MY_IP);
    //send_rrep(&entry_1, &targNode);
    END_TEST();
    
    test_packets_cleanup();
}

static void test_seqNum(void){
    init_seqNum();
    uint16_t seqNum;
    int seqNum_cmp, test;

    START_TEST();

    test_seqnum_basic_functionality();
    test_seqnum_comparison_around_tipping_point();
    test_seqnum_tipping();  

    END_TEST();
}

static void test_packets_init(void)
{
    reader_init();
    writer_init(test_write_packet);

    init_routingtable();
    init_clienttable();
    init_rreqtable();
}

static void test_packets_cleanup(void)
{
    reader_cleanup();
    writer_cleanup();
    abuf_free(&_hexbuf);
}

/**
 * Handle the output of the RFC5444 packet creation process
 * @param wr
 * @param iface
 * @param buffer
 * @param length
 */
static void
test_write_packet(struct rfc5444_writer *wr __attribute__ ((unused)),
        struct rfc5444_writer_target *iface __attribute__((unused)),
        void *buffer, size_t length)
{

    /* generate hexdump and human readable representation of packet
        and print to console */
    abuf_hexdump(&_hexbuf, "\t", buffer, length);
    rfc5444_print_direct(&_hexbuf, buffer, length);

    printf("%s", abuf_getptr(&_hexbuf));
    abuf_clear(&_hexbuf);

    /* parse packet */
    struct netaddr addr;
    netaddr_from_string(&addr, "::111");
    reader_handle_packet(buffer, length, &addr);
}


//int main(int argc __attribute__ ((unused)), char **argv __attribute__ ((unused))) {
void test_packets_main(void)
{
    netaddr_from_string(&addr, "::111");
    
    BEGIN_TESTING(NULL);
    
    test_rreq_dropped();
    //test_rreq_to_rrep();
    //test_rrep();
    //test_seqNum();

    FINISH_TESTING();
}