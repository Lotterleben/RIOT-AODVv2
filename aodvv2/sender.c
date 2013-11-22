#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "vtimer.h"
#include "rtc.h"
#include "board_uart0.h"
#include "shell.h"
#include "shell_commands.h"
#include "board.h"
#include "transceiver.h"
#include "posix_io.h"
#include "nativenet.h"
#include "msg.h"
#include <thread.h>

#include "common/common_types.h"
#include "common/netaddr.h"

#include "ltc4150.h"
#include "rfc5444/rfc5444_reader.h"
#include "rfc5444/rfc5444_writer.h"
#include "rfc5444/rfc5444_print.h"

#include "reader.h"
#include "aodvv2_writer.h"
#include "sender.h"
#include "destiny.h"
#include "destiny/socket.h"
#include "net_help.h"

#include "sender.h"
#include "include/aodvv2.h"

#define SND_BUFFER_SIZE     (100)
#define RCV_BUFFER_SIZE     (64)
#define RADIO_STACK_SIZE    (KERNEL_CONF_STACKSIZE_DEFAULT)

char radio_stack_buffer[RADIO_STACK_SIZE];
msg_t msg_q[RCV_BUFFER_SIZE];
static struct autobuf _hexbuf;

int sock;
sockaddr6_t sockaddr;

/* helper methods */
void init_writer(void);
void radio(void);
static void write_packet(struct rfc5444_writer *wr __attribute__ ((unused)),
        struct rfc5444_writer_target *iface __attribute__((unused)),
        void *buffer, size_t length);


void aodv_init(void){
 
    /* initialize buffer for hexdump */
    abuf_init(&_hexbuf);

    /* init reader and writer */
    reader_init();
    writer_init(write_packet);

    /* init (broadcast) address */
    sockaddr.sin6_family = AF_INET;
    sockaddr.sin6_port = MANET_PORT;

    /* init socket */
    sock = destiny_socket(PF_INET6, SOCK_DGRAM, IPPROTO_UDP);

    printf("[sender.c] sock: %i\n", sock);

    if(-1 == sock) {
        printf("Error Creating Socket!");
        exit(EXIT_FAILURE);
    }

}

void send_udp(void *buffer, size_t length){
//void send_udp(char *str){

    timex_t start, end, total;
    long secs;

    int bytes_sent;
    int address, count;
    //char text[] = "abcdefghijklmnopqrstuvwxyz0123456789!-=$%&/()";
    //sscanf(str, "send_udp %i %i %s", &count, &address, text);

    printf("sending data...\n");
    bytes_sent = destiny_socket_sendto(sock, buffer, length, 0, &sockaddr, 
                                       sizeof sockaddr);

    if(bytes_sent < 0) {
        printf("Error sending packet: % i\n", bytes_sent);
    }else{
        printf("success!\n");
    }

    // übergangslösung
    destiny_socket_close(sock);
}

void send_rreq(char *str){
    printf("[aodvv2] sending RREQ...\n");

    writer_send_rreq();

    /* cleanup */
    reader_cleanup();
    writer_cleanup();
    abuf_free(&_hexbuf);

    printf("[aodvv2] RREQ sent\n");
}

void send_rrep(char *str){
    printf("[aodvv2] sending RREP...\n");

    writer_send_rrep();

    /* cleanup */
    reader_cleanup();
    writer_cleanup();
    abuf_free(&_hexbuf);

    printf("[aodvv2] RREP sent\n");
}

/*********** HELPERS **********************************************************/

void init_writer(void){

}

void radio(void) {
    msg_t m;
    radio_packet_t *p;
    uint8_t i;

    msg_init_queue(msg_q, RCV_BUFFER_SIZE);

    while (1) {
        msg_receive(&m);
        if (m.type == PKT_PENDING) {
            p = (radio_packet_t*) m.content.ptr;
            printf("Got radio packet:\n");
            printf("\tLength:\t%u\n", p->length);
            printf("\tSrc:\t%u\n", p->src);
            printf("\tDst:\t%u\n", p->dst);
            printf("\tLQI:\t%u\n", p->lqi);
            printf("\tRSSI:\t%u\n", p->rssi);

            for (i = 0; i < p->length; i++) {
                printf("%02X ", p->data[i]);
            }
            p->processing--;
            puts("\n");
        }
        else if (m.type == ENOBUFFER) {
            puts("Transceiver buffer full");
        }
        else {
            puts("Unknown packet received");
        }
    }
}

/**
 * Handle the output of the RFC5444 packet creation process
 * @param wr
 * @param iface
 * @param buffer
 * @param length
 */
static void
write_packet(struct rfc5444_writer *wr __attribute__ ((unused)),
        struct rfc5444_writer_target *iface __attribute__((unused)),
        void *buffer, size_t length) {
    printf("[aodvv2] %s()\n", __func__);

    /* generate hexdump and human readable representation of packet
        and print to console */
    abuf_hexdump(&_hexbuf, "\t", buffer, length);
    rfc5444_print_direct(&_hexbuf, buffer, length);
    printf("%s", abuf_getptr(&_hexbuf));

    /* parse packet */
    //rfc5444_reader_handle_packet(&reader, buffer, length);
    send_udp(buffer, length);
    //send_udp("");
}



