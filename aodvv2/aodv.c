#include "include/aodvv2.h"
#include "seqnum.h"
#include "routing.h" 
#include "utils.h"
#include "reader.h"
#include "writer.h"

static void write_packet(struct rfc5444_writer *wr __attribute__ ((unused)),
    struct rfc5444_writer_target *iface __attribute__((unused)),
    void *buffer, size_t length);

void aodv_init(void)
{
    init_seqNum();
    init_routingtable();
    init_clienttable();
    init_rreqtable();

    reader_init();
    writer_init(write_packet);
}


static void write_packet(struct rfc5444_writer *wr __attribute__ ((unused)),
    struct rfc5444_writer_target *iface __attribute__((unused)),
    void *buffer, size_t length) {

    //TODO
}