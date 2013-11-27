#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <mutex.h>

#include "vtimer.h"
#include "rtc.h"
#include "board_uart0.h"
#include "shell.h"
#include "shell_commands.h"
#include "board.h"
#include "posix_io.h"
#include "nativenet.h"
#include "msg.h"
#include <thread.h>

#include "common/common_types.h"
#include "common/netaddr.h"

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


static struct autobuf _hexbuf;

static int sock;
static sockaddr6_t sockaddr;

static uint16_t seqNum; 

/* helper methods */
void init_writer(void);
void send_udp(void *buffer, size_t length);
static void write_packet(struct rfc5444_writer *wr __attribute__ ((unused)),
        struct rfc5444_writer_target *iface __attribute__((unused)),
        void *buffer, size_t length);
static void print_ipv6_addr(const ipv6_addr_t *ipv6_addr);


void aodv_init(void)
{   
    test = 1;
    /* initialize sequence number and its mutex */
    mutex_init(&m_seqnum);
    // TODO: overkill?
    mutex_lock(&m_seqnum);
    seqNum = 1;
    mutex_unlock(&m_seqnum);

    /* initialize buffer for hexdump */
    abuf_init(&_hexbuf);

    /* init reader and writer */
    reader_init();
    writer_init(write_packet);

    /* init (broadcast) address */
    sockaddr.sin6_family = AF_INET6;
    sockaddr.sin6_port = MANET_PORT;
    ipv6_addr_set_all_nodes_addr(&sockaddr.sin6_addr); // TODO: does this make sense?

    //printf("[aodvv2] My IP address is:\n");
    //print_ipv6_addr(&sockaddr.sin6_addr);

    /* init socket */
    sock = destiny_socket(PF_INET6, SOCK_DGRAM, IPPROTO_UDP);

    if(-1 == sock) {
        printf("Error Creating Socket!");
        exit(EXIT_FAILURE);
    }
}

/*********** COMMANDS *********************************************************/

void send_rreq(char *str)
{
    printf("[aodvv2] sending RREQ...\n");

    writer_send_rreq();

    /* cleanup */
    reader_cleanup();
    writer_cleanup();
    abuf_free(&_hexbuf);

    printf("[aodvv2] RREQ sent\n");
}

void send_rrep(char *str)
{
    printf("[aodvv2] sending RREP...\n");

    writer_send_rrep();

    /* cleanup */
    reader_cleanup();
    writer_cleanup();
    abuf_free(&_hexbuf);

    printf("[aodvv2] RREP sent\n");
}

void receive_udp(char *str)
{
    sockaddr6_t sa = {0};
    sa.sin6_family = AF_INET6;
    sa.sin6_port = MANET_PORT;

    uint32_t fromlen;
    int32_t recvsize; 
    char buffer[256];

    printf("initializing UDP server...\n");
    sock = destiny_socket(PF_INET6, SOCK_DGRAM, IPPROTO_UDP);
    fromlen = sizeof(sa);

    if(destiny_socket_bind(sock, &sa, sizeof(sa)) < 0 ) {
        printf("Error: bind failed! Exiting.\n");
        destiny_socket_close(sock);
        exit(EXIT_FAILURE);
    }

    printf("ready to receive packets.\n");

    for(;;){
        recvsize = destiny_socket_recvfrom(sock, (void *)buffer, sizeof(buffer), 0, 
                                            &sa, &fromlen);

        if(recvsize < 0) {
            printf("Error receiving data!\n");
        }

        printf("recvsize: %"PRIi32"\n ", recvsize);
        printf("datagram: %s\n", buffer);
    }
}

/*********** HELPERS **********************************************************/

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
        void *buffer, size_t length)
{
    printf("[aodvv2] %s()\n", __func__);

    /* generate hexdump and human readable representation of packet
        and print to console */
    abuf_hexdump(&_hexbuf, "\t", buffer, length);
    rfc5444_print_direct(&_hexbuf, buffer, length);
    printf("%s", abuf_getptr(&_hexbuf));

    send_udp(buffer, length);
}

void send_udp(void *buffer, size_t length)
{
    int bytes_sent;

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

void inc_seqNum(void)
{
    mutex_lock(&m_seqnum);
    //printf("[aodvv2] %s(): seqNum = %i\n", __func__, seqNum);
    seqNum++;
    //printf("[aodvv2] %s(): seqNum = %i\n", __func__, seqNum);
    mutex_unlock(&m_seqnum);
}

/* if I use a mutex here, that smells like race conditions, doesn't it?
Am Besten m_seqNum doch static machen und dann drauf locken wenn 
ich seqnum auslese... */
uint16_t get_seqNum(void)
{
    return seqNum;
}

void print_ipv6_addr(const ipv6_addr_t *ipv6_addr)
{
    char addr_str[IPV6_MAX_ADDR_STR_LEN];
    printf("%s\n", ipv6_addr_to_str(addr_str, ipv6_addr));
}

