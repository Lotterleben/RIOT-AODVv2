#include <stdio.h>
#include <string.h>
#include <unistd.h> // for getting the pid
#include <inet_pton.h>
#include <time.h>
#include <stdlib.h>

#include "thread.h"
#include "posix_io.h"
#include "shell.h"
#include "shell_commands.h"
#include "board_uart0.h"
#include "udp.h"
#include "net_help.h"

#include "kernel.h"
#include <config.h>

#include "aodvv2/aodvv2.h"
#include "nativenet.h"

#define ENABLE_DEBUG (1)
#include "debug.h"

#define RANDOM_PORT         (1337)
#define UDP_BUFFER_SIZE     (128)
#define RCV_MSG_Q_SIZE      (64)
#define DATA_SIZE           (20)
#define STREAM_INTERVAL     (2000000)     // microseconds
#define NUM_PKTS            (100)

// constants from the AODVv2 Draft, version 03
#define DISCOVERY_ATTEMPTS_MAX (3) //(3)
#define RREQ_WAIT_TIME         (2000000) // microseconds = 2 seconds

int demo_attempt_to_send(char* dest_str, char* msg);

static int _sock_snd, if_id;
static sockaddr6_t _sockaddr;
static ipv6_addr_t prefix;

msg_t msg_q[RCV_MSG_Q_SIZE];
char addr_str[IPV6_MAX_ADDR_STR_LEN];
char _rcv_stack_buf[KERNEL_CONF_STACKSIZE_MAIN];
timex_t now;

uint16_t get_hw_addr(void)
{
    return sysconfig.id;
}

void demo_send(int argc, char** argv)
{
    if (argc != 3) {
        printf("Usage: send <destination ip> <message>\n");
        return;
    }

    char* dest_str = argv[1] ;
    char* msg = argv[2];

    demo_attempt_to_send(dest_str, msg);
}

/*
    Send a random 20-byte chunk of data to the address supplied by the user
*/
void demo_send_data(int argc, char** argv)
{
    if (argc != 2) {
        printf("Usage: send_data <destination ip>\n");
        return;
    }

    demo_attempt_to_send(argv[1], "This is a test");
}

void demo_send_stream(int argc, char** argv)
{
    if (argc != 2) {
        printf("Usage: send_stream <destination ip>\n");
        return;
    }

    char* dest_str = argv[1];

    int msg_size = sizeof(char)*81;
    char* msg = (char*) malloc(msg_size);

    memset(msg, 'a', msg_size);
    msg[msg_size - 1] = '\0';

    /* TODO un-uncomment me
    if (demo_attempt_to_send(dest_str, msg) < 0 ){
        printf("[demo]   No route found, can't stream data.\n");
        return;
    }
    */
    inet_pton(AF_INET6, dest_str, &_sockaddr.sin6_addr);
    int msg_len = strlen(msg)+1;

    vtimer_now(&now);
    printf("{%" PRIu32 ":%" PRIu32 "}[demo]   sending stream of %i bytes...\n", now.seconds, now.microseconds, sizeof(msg) * strlen(msg));

    for (int i=0; i < NUM_PKTS; i++) {
        vtimer_now(&now);

        printf("{%" PRIu32 ":%" PRIu32 "}[demo]   sending packet of %i bytes towards %s...\n", now.seconds, now.microseconds, msg_len, dest_str);

        socket_base_sendto(_sock_snd, msg, msg_len, 0, &_sockaddr, sizeof _sockaddr);

        vtimer_usleep(STREAM_INTERVAL);
        printf("%i\n", i);
    }
    free(msg);
}

/*
    Help emulate a functional NDP implementation (this should be called by every
    neighbor of a node that was shut down with demo_exit())
*/
void demo_remove_neighbor(int argc, char** argv)
{
    if (argc != 2) {
        printf("Usage: rm_neighbor <destination ip>\n");
        return;
    }
    ipv6_addr_t neighbor;
    ndp_neighbor_cache_t* nc_entry;
    inet_pton(AF_INET6, argv[1], &neighbor);
    nc_entry = ndp_neighbor_cache_search(&neighbor);
    if (nc_entry) {
        nc_entry->state = NDP_NCE_STATUS_INCOMPLETE;
        printf("[demo] neighbor removed.\n");
    }
    else {
        printf("[demo] couldn't remove neighbor.\n");
    }
}

/*
    Help emulate a functional NDP implementation (this should be called for every
    neighbor of the node on the grid)
*/
void demo_add_neighbor(int argc, char** argv)
{
    if (argc != 3) {
        printf("Usage: add_neighbor <neighbor ip> <neighbor ll-addr>\n");
        return;
    }

    net_if_eui64_t eut_eui64;
    ipv6_addr_t neighbor;
    inet_pton(AF_INET6, argv[1], &neighbor);

    // only add neighbor if it's not already in Cache
    if (ndp_neighbor_cache_search(&neighbor)!= NULL){
        printf("IP %s already in Neighbor Cache, lladdr-len:%i\n", ipv6_addr_to_str(addr_str, IPV6_MAX_ADDR_STR_LEN, &neighbor), ndp_neighbor_cache_search(&neighbor)->lladdr_len);
        return;
    }

    /* convert & flip */
    memcpy(&eut_eui64, &neighbor.uint8[8], 8);
    eut_eui64.uint8[0] ^= 0x02;

    ndp_neighbor_cache_add(0, &neighbor, &neighbor.uint16[7], 2, 0, NDP_NCE_STATUS_REACHABLE,
                                  NDP_NCE_TYPE_TENTATIVE, 0xffff);

    printf("neighbor added.\n");
}

void demo_exit(int argc, char** argv)
{
    exit(0);
}

int demo_attempt_to_send(char* dest_str, char* msg)
{
    uint8_t num_attempts = 0;

    // turn dest_str into ipv6_addr_t
    inet_pton(AF_INET6, dest_str, &_sockaddr.sin6_addr);
    int msg_len = strlen(msg)+1;

    vtimer_now(&now);
    printf("{%" PRIu32 ":%" PRIu32 "}[demo]   sending packet of %i bytes towards %s...\n", now.seconds, now.microseconds, msg_len, dest_str);

    while(num_attempts < DISCOVERY_ATTEMPTS_MAX) {
        int bytes_sent = socket_base_sendto(_sock_snd, msg, msg_len,
                                                0, &_sockaddr, sizeof _sockaddr);

        vtimer_now(&now);
        if (bytes_sent == -1) {
            printf("{%" PRIu32 ":%" PRIu32 "}[demo]   no bytes sent, probably because there is no route yet.\n", now.seconds, now.microseconds);
            num_attempts++;
            vtimer_usleep(RREQ_WAIT_TIME);
        }
        else {
            printf("{%" PRIu32 ":%" PRIu32 "}[demo]   Success sending Data: %d bytes sent.\n", now.seconds, now.microseconds, bytes_sent);
            return 0;
        }
    }
    //printf("{%" PRIu32 ":%" PRIu32 "}[demo]  Error sending Data: no route found\n", now.seconds, now.microseconds);
    return -1;
}

void demo_print_routingtable(int argc, char** argv)
{
    print_routingtable();
}

static void _demo_init_socket(void)
{
    _sockaddr.sin6_family = AF_INET6;
    _sockaddr.sin6_port = HTONS(RANDOM_PORT);

    _sock_snd = socket_base_socket(PF_INET6, SOCK_DGRAM, IPPROTO_UDP);

    if(-1 == _sock_snd) {
        printf("[demo]   Error Creating Socket!\n");
        return;
    }
}

static void *_demo_receiver_thread(void *arg)
{
    uint32_t fromlen;
    int32_t rcv_size;
    char buf_rcv[UDP_BUFFER_SIZE];
    char addr_str_rec[IPV6_MAX_ADDR_STR_LEN];
    msg_t rcv_msg_q[RCV_MSG_Q_SIZE];


    timex_t now;

    msg_init_queue(rcv_msg_q, RCV_MSG_Q_SIZE);

    sockaddr6_t sa_rcv = { .sin6_family = AF_INET6,
                           .sin6_port = HTONS(RANDOM_PORT) };

    int sock_rcv = socket_base_socket(PF_INET6, SOCK_DGRAM, IPPROTO_UDP);

    if (-1 == socket_base_bind(sock_rcv, &sa_rcv, sizeof(sa_rcv))) {
        DEBUG("[demo]   Error: bind to receive socket failed!\n");
        socket_base_close(sock_rcv);
    }

    DEBUG("[demo]   ready to receive data\n");
    for(;;) {
        rcv_size = socket_base_recvfrom(sock_rcv, (void *)buf_rcv, UDP_BUFFER_SIZE, 0,
                                          &sa_rcv, &fromlen);

        vtimer_now(&now);

        if(rcv_size < 0) {
            DEBUG("{%" PRIu32 ":%" PRIu32 "}[demo]   ERROR receiving data!\n", now.seconds, now.microseconds);
        }
        DEBUG("{%" PRIu32 ":%" PRIu32 "}[demo]   UDP packet received from %s: %s\n", now.seconds, now.microseconds, ipv6_addr_to_str(addr_str_rec, IPV6_MAX_ADDR_STR_LEN, &sa_rcv.sin6_addr), buf_rcv);
    }

    socket_base_close(sock_rcv);
}

/* init transport layer & routing stuff*/
static void _init_tlayer()
{
    msg_init_queue(msg_q, RCV_MSG_Q_SIZE);

    net_if_set_hardware_address(0, get_hw_addr());

    printf("initializing 6LoWPAN...\n");

    ipv6_addr_init(&prefix, 0xABCD, 0xEF12, 0, 0, 0, 0, 0, 0);
    if_id = 0; // >1 interface isn't supported anyway, so there

    //sixlowpan_lowpan_init_adhoc_interface(if_id, &prefix);
    sixlowpan_lowpan_init_interface(if_id);
    printf("initializing AODVv2...\n");

    aodv_init();
    _demo_init_socket();
}

void demo_print_transceiverbuffer(int argc, char** argv)
{
    DEBUG("TRANSCEIVER_BUFFER_SIZE: %d \n", TRANSCEIVER_BUFFER_SIZE);
}

const shell_command_t shell_commands[] = {
    {"print_rt", "print routingtable", demo_print_routingtable},
    {"send", "send message to ip", demo_send},
    {"send_data", "send 20 bytes of data to ip", demo_send_data},
    {"send_stream", "send stream of data to ip", demo_send_stream},
    {"add_neighbor", "add neighbor to Neighbor Cache", demo_add_neighbor},
    {"rm_neighbor", "remove neighbor from Neighbor Cache", demo_remove_neighbor},
    {"tb_size", "print sie of transceiver buffer", demo_print_transceiverbuffer},
    {"exit", "Shut down the RIOT", demo_exit},
    {NULL, NULL, NULL}
};

int main(void)
{
    _init_tlayer();
    thread_create(_rcv_stack_buf, KERNEL_CONF_STACKSIZE_MAIN, PRIORITY_MAIN, CREATE_STACKTEST, _demo_receiver_thread, NULL ,"_demo_receiver_thread");

    posix_open(uart0_handler_pid, 0);

    printf("\n\t\t\tWelcome to RIOT\n\n");

    shell_t shell;
    shell_init(&shell, shell_commands, UART0_BUFSIZE, uart0_readc, uart0_putc);

    shell_run(&shell);

    return 0;
}
