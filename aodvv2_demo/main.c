/*
 * Copyright (C) 2015
 *
 * This file is subject to the terms and conditions of the GNU Lesser
 * General Public License v2.1. See the file LICENSE in the top level
 * directory for more details.
 */

/**
 * @ingroup     examples
 * @{
 *
 * @file
 * @brief
 *
 * @author Martin Landsmann <Martin.Landsmann@HAW-Hamburg.de>
 * @author Lotte Steenbrink <lotte.steenbrink@haw-hamburg.de>
 *
 * @}
 */

#include <stdio.h>
#include "net/ng_ipv6.h"
#include "net/ng_netbase.h"
#include "net/ng_netif.h"
#include "net/ng_udp.h"
#include "net/ng_pkt.h"
#include "shell.h"
#include "transceiver.h"
#include "board_uart0.h"
#include "posix_io.h"

#include "aodvv2/aodvv2.h"

#define RANDOM_PORT         (1337)
#define UDP_BUFFER_SIZE     (128) /** with respect to IEEE 802.15.4's MTU */
#define RCV_MSG_Q_SIZE      (32)  /* TODO: check if smaller values work, too */

char _rcv_stack_buf[THREAD_STACKSIZE_MAIN];


/* init our network stack */
int init_network(void) {
    /** We take the first available IF */
    kernel_pid_t ifs[NG_NETIF_NUMOF];
    size_t numof = ng_netif_get(ifs);
    if(numof <= 0) {
        return 1;
    }

#ifndef BOARD_NATIVE
    /** We set our channel */
    uint16_t data = 17;
    if (ng_netapi_set(ifs[0], NETCONF_OPT_CHANNEL, 0, &data, sizeof(uint16_t)) < 0) {
        return 1;
    }
    /** We set our pan ID */
    data = 0xabcd;
    if (ng_netapi_set(ifs[0], NETCONF_OPT_NID, 0, &data, sizeof(uint16_t)) < 0) {
        return 1;
    }
#endif

    ng_ipv6_addr_t addr;
    uint8_t prefix_len = 128;
    char* addr_str = "2001::1234";
    if (ng_ipv6_addr_from_str(&addr, addr_str) == NULL) {
        puts("error: unable to parse IPv6 address.");
        return 1;
    }

    if (ng_ipv6_netif_add_addr(ifs[0], &addr, prefix_len, NG_IPV6_NETIF_ADDR_FLAGS_UNICAST) == NULL) {
        puts("error: unable to add IPv6 address\n");
        return 1;
    }

    aodv_init();

    return 0;
}

/**
    M. BEGIN STOLEN FROM:
    examples/ng_networking/udp.c:34 - :90
*/
static void send(ng_ipv6_addr_t addr, uint16_t port, void *data, size_t data_length)
{
    ng_pktsnip_t *payload, *udp, *ip;
    ng_netreg_entry_t *sendto;

    /* convert to correct byteorder */
    port = HTONS(port);

    /* allocate payload */
    payload = ng_pktbuf_add(NULL, data, data_length, NG_NETTYPE_UNDEF);
    if (payload == NULL) {
        puts("Error: unable to copy data to packet buffer");
        return;
    }
    /* allocate UDP header, set source port := destination port TODO is this such a good idea?? */
    udp = ng_udp_hdr_build(payload, (uint8_t*)&port, 2, (uint8_t*)&port, 2);
    if (udp == NULL) {
        puts("Error: unable to allocate UDP header");
        ng_pktbuf_release(payload);
        return;
    }
    /* allocate IPv6 header */
    ip = ng_ipv6_hdr_build(udp, NULL, 0, (uint8_t *)&addr, sizeof(addr));
    if (ip == NULL) {
        puts("Error: unable to allocate IPv6 header");
        ng_pktbuf_release(udp);
        return;
    }
    /* send packet */
    sendto = ng_netreg_lookup(NG_NETTYPE_UDP, NG_NETREG_DEMUX_CTX_ALL);
    if (sendto == NULL) {
        puts("Error: unable to locate UDP thread");
        ng_pktbuf_release(ip);
        return;
    }
    ng_pktbuf_hold(ip, ng_netreg_num(NG_NETTYPE_UDP,
                                     NG_NETREG_DEMUX_CTX_ALL) - 1);
    while (sendto != NULL) {
        ng_netapi_send(sendto->pid, ip);
        sendto = ng_netreg_getnext(sendto);
    }
}
/**
    M. END STOLEN FROM:
    examples/ng_networking/udp.c:34 - :90
*/


int demo_send(int argc, char** argv)
{
    if (argc != 3) {
        printf("Usage: send <destination ip> <message>\n");
        return 1;
    }

    char* dest_str = argv[1] ;
    char* msg = argv[2];
    ng_ipv6_addr_t ng_addr;

    /* turn dest_str into ng_ipv6_addr_t */
    ng_ipv6_addr_from_str(&ng_addr, dest_str);
    int msg_len = strlen(msg)+1;

    printf("[demo]   sending packet of %i bytes towards %s...\n", msg_len, dest_str);
    send(ng_addr, RANDOM_PORT, msg, msg_len);

    return 0;
}

static void *_demo_receiver_thread(void *arg)
{
    (void) arg;
    ng_netreg_entry_t server = {
        .next = NULL,
        .demux_ctx = NG_NETREG_DEMUX_CTX_ALL,
        .pid = KERNEL_PID_UNDEF };

    msg_t msg, reply;
    msg_t msg_q[RCV_MSG_Q_SIZE];

    msg_init_queue(msg_q, RCV_MSG_Q_SIZE);

    reply.content.value = (uint32_t)(-ENOTSUP);
    reply.type = NG_NETAPI_MSG_TYPE_ACK;

    /* start server (which means registering AODVv2 receiver for the chosen port) */
    server.pid = sched_active_pid; /* sched_active_pid is our pid, since we are currently act */
    server.demux_ctx = (uint32_t)HTONS(RANDOM_PORT);
    ng_netreg_register(NG_NETTYPE_UDP, &server);

    ng_pktsnip_t *pkt;

    while (1) {
        msg_receive(&msg);

        switch (msg.type) {
            case NG_NETAPI_MSG_TYPE_RCV:
                pkt = (ng_pktsnip_t *)msg.content.ptr;
                printf("[demo] received data: %s\n", (char*) pkt->data);
                ng_pktbuf_release(pkt);
                break;
            case NG_NETAPI_MSG_TYPE_SND:
                break;
            case NG_NETAPI_MSG_TYPE_GET:
            case NG_NETAPI_MSG_TYPE_SET:
                msg_reply(&msg, &reply);
                break;
            default:
                printf("[demo] received something unexpected\n");
                break;
        }
    }

    return NULL;
}


const shell_command_t shell_commands[] = {
    {"send", "send message to ip", demo_send},
    {NULL, NULL, NULL}
};

int main(void)
{
    if (init_network() != 0) {
        return -1;
    }
    thread_create(_rcv_stack_buf, sizeof(_rcv_stack_buf), THREAD_PRIORITY_MAIN, CREATE_STACKTEST, _demo_receiver_thread, NULL ,"_demo_receiver_thread");

    /* TODO Do I still need this? */
    posix_open(uart0_handler_pid, 0);

    printf("\n\t\t\tWelcome to RIOT\n\n");

    shell_t shell;
    shell_init(&shell, shell_commands, UART0_BUFSIZE, uart0_readc, uart0_putc);

    shell_run(&shell);
}
