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
#include "net/gnrc/netif.h"
#include "net/gnrc/udp.h"
#include "net/gnrc/ipv6.h"
#include "net/gnrc/pkt.h"
#include "shell.h"


#include "aodvv2/aodvv2.h"

#define RANDOM_PORT         (1337)
#define UDP_BUFFER_SIZE     (128) /** with respect to IEEE 802.15.4's MTU */
#define RCV_MSG_Q_SIZE      (32)  /* TODO: check if smaller values work, too */

char rcv_stack_buf[THREAD_STACKSIZE_MAIN];
char line_buf[SHELL_DEFAULT_BUFSIZE];

static void send_data(char *addr_str, char *port_str, char *data)
{
    uint8_t port[2];
    uint16_t tmp;
    gnrc_pktsnip_t *payload, *udp, *ip;
    ipv6_addr_t addr;
    gnrc_netreg_entry_t *sendto_;

    /* parse destination address */
    if (ipv6_addr_from_str(&addr, addr_str) == NULL) {
        puts("Error: unable to parse destination address");
        return;
    }
    /* parse port */
    tmp = (uint16_t)atoi(port_str);
    if (tmp == 0) {
        puts("Error: unable to parse destination port");
        return;
    }
    port[0] = (uint8_t)tmp;
    port[1] = tmp >> 8;

    /* allocate payload */
    payload = gnrc_pktbuf_add(NULL, data, strlen(data), GNRC_NETTYPE_UNDEF);
    if (payload == NULL) {
        puts("Error: unable to copy data to packet buffer");
        return;
    }
    /* allocate UDP header, set source port := destination port */
    udp = gnrc_udp_hdr_build(payload, port, 2, port, 2);
    if (udp == NULL) {
        puts("Error: unable to allocate UDP header");
        gnrc_pktbuf_release(payload);
        return;
    }
    /* allocate IPv6 header */
    ip = gnrc_ipv6_hdr_build(udp, NULL, 0, (uint8_t *)&addr, sizeof(addr));
    if (ip == NULL) {
        puts("Error: unable to allocate IPv6 header");
        gnrc_pktbuf_release(udp);
        return;
    }
    /* send packet */
    sendto_ = gnrc_netreg_lookup(GNRC_NETTYPE_UDP, GNRC_NETREG_DEMUX_CTX_ALL);
    if (sendto_ == NULL) {
        puts("Error: unable to locate UDP thread");
        gnrc_pktbuf_release(ip);
        return;
    }
    gnrc_pktbuf_hold(ip, gnrc_netreg_num(GNRC_NETTYPE_UDP,
                                     GNRC_NETREG_DEMUX_CTX_ALL) - 1);
    while (sendto_ != NULL) {
        gnrc_netapi_send(sendto_->pid, ip);
        sendto_ = gnrc_netreg_getnext(sendto_);
    }

    // TODO: don't I have to release ip here as well?

    printf("Success: send %i byte to %s:%u\n", payload->size, addr_str, tmp);
}


int demo_send(int argc, char** argv)
{
    if (argc != 3) {
        printf("Usage: send <destination ip> <message>\n");
        return 1;
    }

    /*
    char* dest_str = argv[1] ;
    char* msg = argv[2];


    ng_ipv6_addr_t ng_addr;

    ng_ipv6_addr_from_str(&ng_addr, dest_str);
    int msg_len = strlen(msg);//+1;
    printf("[demo]   sending packet of %i bytes towards %s...\n", msg_len, dest_str);

    */
    send_data(argv[1], "1234", argv[2]);

    return 0;
}

/*
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

    // start server (which means registering AODVv2 receiver for the chosen port)
    server.pid = sched_active_pid; // sched_active_pid is our pid, since we are currently act
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
*/


const shell_command_t shell_commands[] = {
    {"send", "send message to ip", demo_send},
    {NULL, NULL, NULL}
};

int main(void)
{
    /* get interface on which aodv is running */
    kernel_pid_t ifs[GNRC_NETIF_NUMOF];
    size_t numof = gnrc_netif_get(ifs);
    if(numof <= 0) {
        printf("no interface available: dropping packet.");
        return -1;
    }
    /* use the first interface */
    aodv_init(ifs[0]);

    //thread_create(rcv_stack_buf, sizeof(_rcv_stack_buf), THREAD_PRIORITY_MAIN, CREATE_STACKTEST, _demo_receiver_thread, NULL ,"_demo_receiver_thread");

    printf("\n\t\t\tWelcome to RIOT\n\n");

    shell_run(shell_commands, line_buf, SHELL_DEFAULT_BUFSIZE);
}
