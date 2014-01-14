#include <stdio.h>

#include "cunit/cunit.h"
#include "common/netaddr.h"
#include "aodvv2_writer.h"

// 2nd try.
static struct autobuf _hexbuf;
void* _buffer;
size_t _length;


void test_rreq(void)
{
    struct netaddr origNode, targNode;
    netaddr_from_string(&origNode, "::13");
    netaddr_from_string(&targNode, "::14");

    START_TEST();
    writer_send_rreq(&origNode, &targNode);
    END_TEST();
}

static void
test_write_packet(struct rfc5444_writer *wr __attribute__ ((unused)),
        struct rfc5444_writer_target *iface __attribute__((unused)),
        void *buffer, size_t length)
{
    /* generate hexdump and human readable representation of packet
        and print to console */
    _buffer = buffer;
    _length = length;

    /* Wenn ich das hier rausnehme und in test_rreq packe
    geht mir der Kontext verloren (oder so)
    und er erkennt nur noch dass es ein paket ist.. gnahhh */
    abuf_hexdump(&_hexbuf, "\t", _buffer, _length);
    rfc5444_print_direct(&_hexbuf, _buffer, _length);
    printf("%s", abuf_getptr(&_hexbuf));
    abuf_clear(&_hexbuf);

    /* Selbes Problem hier: er hält's für nen unbrauchbaren datenblob. */
    struct netaddr addr;
    netaddr_from_string(&addr, "::111");
    reader_handle_packet(_buffer, _length, &addr);

}


void test_packets_main(void)
{
    reader_init();
    writer_init(test_write_packet);

    BEGIN_TESTING(NULL);
    test_rreq();
    FINISH_TESTING();
}
