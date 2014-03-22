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
#include "destiny.h"
#include "net_help.h"

#include "kernel.h"

#include "aodvv2/aodvv2.h"

#define ENABLE_DEBUG (1)
#include "debug.h"

#define RANDOM_PORT         (1337)
#define UDP_BUFFER_SIZE     (128)
#define RCV_MSG_Q_SIZE      (64)
#define DATA_SIZE           (20)
#define STREAM_INTERVAL     (200000)     // milliseconds
#define NUM_PKTS            (100)

// constants from the AODVv2 Draft, version 03
#define DISCOVERY_ATTEMPTS_MAX (3)
#define RREQ_WAIT_TIME         (2000000) // milliseconds

int demo_attempt_to_send(char* dest_str, char* msg);

static int _sock_snd, if_id;
static sockaddr6_t _sockaddr;
static ipv6_addr_t prefix;
static uint8_t random_data[DATA_SIZE];  // assuming we're on a platform where 8 bit == 1 byte

msg_t msg_q[RCV_MSG_Q_SIZE];

char _rcv_stack_buf[KERNEL_CONF_STACKSIZE_MAIN];


// TODO ifdef for native, msba2.. etc. getpid() geht nur für native
/* martine:für den iot-lab generier ich das immer aus der Serialnumber der 
cpu und für native aus der PID. Ich weiß, dass der MSBA2 auch irgendwo eine 
Serialnumber hat, aber das letzte mal als ich das probiert hatte, kam es zu 
segfaults an genau der stelle des auslesens
aber afaik gibts in core/include/config.h was mit id… aber kA wie zuverlässig das ist
*/
uint16_t get_hw_addr(void)
{
    return getpid();
}

void demo_send(int argc, char** argv)
{
    if (argc != 3) {
        printf("Usage: send <destination ip> <message>\n");
        return;
    }

    char* dest_str = argv[1] ;
    char* msg = argv[2];
    uint8_t num_attempts = 0;
    
    printf("[demo]   sending packet of %i bytes...\n", sizeof(msg) * strlen(msg));

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

    // get some random data
    char* msg = (char*) malloc(sizeof(char)*81); 
    char* dest_str = argv[1];

    /* TODO un-uncomment me
    if (demo_attempt_to_send(dest_str, msg) < 0 ){
        printf("[demo]   No route found, can't stream data.\n");
        return;
    }
    */

    inet_pton(AF_INET6, dest_str, &_sockaddr.sin6_addr);

    for (int i=0; i < NUM_PKTS; i++) {
        int msg_len = strlen(msg)+1;
        int bytes_sent = destiny_socket_sendto(_sock_snd, msg, msg_len, 
                                                0, &_sockaddr, sizeof _sockaddr);
        vtimer_usleep(STREAM_INTERVAL);
        printf("%i\n", i);
    }
    free(msg);
}

void demo_exit(int argc, char** argv)
{
    exit(0);
}

static void _demo_init_socket(void)
{
    _sockaddr.sin6_family = AF_INET6;
    _sockaddr.sin6_port = HTONS(RANDOM_PORT);

    _sock_snd = destiny_socket(PF_INET6, SOCK_DGRAM, IPPROTO_UDP);

    if(-1 == _sock_snd) {
        printf("[demo]   Error Creating Socket!\n");
        return;
    }
}

int demo_attempt_to_send(char* dest_str, char* msg)
{
    uint8_t num_attempts = 0;

    // turn dest_str into ipv6_addr_t
    inet_pton(AF_INET6, dest_str, &_sockaddr.sin6_addr);

    while(num_attempts < DISCOVERY_ATTEMPTS_MAX) {
        int msg_len = strlen(msg)+1;
        int bytes_sent = destiny_socket_sendto(_sock_snd, msg, msg_len, 
                                                0, &_sockaddr, sizeof _sockaddr);

        if (bytes_sent == -1) {
            printf("[demo]   no bytes sent, probably because there is no route yet.\n");
            num_attempts++;
            vtimer_usleep(RREQ_WAIT_TIME);
        }
        else {
            printf("[demo]   %d bytes sent.\n", bytes_sent);
            return 0;
        }
    }
    printf("[demo]   Error sending Data: no route found\n");
    return -1;
}

static void _demo_receiver_thread(void)
{
    uint32_t fromlen;
    int32_t rcv_size;
    char buf_rcv[UDP_BUFFER_SIZE];
    char addr_str_rec[IPV6_MAX_ADDR_STR_LEN];
    msg_t rcv_msg_q[RCV_MSG_Q_SIZE];
    
    msg_init_queue(rcv_msg_q, RCV_MSG_Q_SIZE);

    sockaddr6_t sa_rcv = { .sin6_family = AF_INET6,
                           .sin6_port = HTONS(RANDOM_PORT) };

    int sock_rcv = destiny_socket(PF_INET6, SOCK_DGRAM, IPPROTO_UDP);
    
    if (-1 == destiny_socket_bind(sock_rcv, &sa_rcv, sizeof(sa_rcv))) {
        DEBUG("[demo]   Error: bind to recieve socket failed!\n");
        destiny_socket_close(sock_rcv);
    }

    DEBUG("[demo]   ready to receive data\n");
    for(;;) {
        rcv_size = destiny_socket_recvfrom(sock_rcv, (void *)buf_rcv, UDP_BUFFER_SIZE, 0, 
                                          &sa_rcv, &fromlen);

        if(rcv_size < 0) {
            DEBUG("[demo]   ERROR receiving data!\n");
        }
        DEBUG("[demo]   UDP packet received from %s: %s\n", ipv6_addr_to_str(addr_str_rec, IPV6_MAX_ADDR_STR_LEN, &sa_rcv.sin6_addr), buf_rcv);
    }

    destiny_socket_close(sock_rcv);  
}

/* init transport layer & routing stuff*/
static void _init_tlayer()
{    
    msg_init_queue(msg_q, RCV_MSG_Q_SIZE);

    net_if_set_hardware_address(0, get_hw_addr());

    printf("initializing 6LoWPAN...\n");

    ipv6_addr_init(&prefix, 0xABCD, 0xEF12, 0, 0, 0, 0, 0, 0);
    if_id = 0; // >1 interface isn't supported anyway, so there

    sixlowpan_lowpan_init_adhoc_interface(if_id, &prefix);
    printf("initializing AODVv2...\n");

    aodv_init();
    _demo_init_socket();
}

const shell_command_t shell_commands[] = {
    {"send", "send message to ip", demo_send},
    {"send_data", "send 20 bytes of data to ip", demo_send_data},
    {"send_stream", "send stream of data to ip", demo_send_stream},
    {"exit", "Shut down the RIOT", demo_exit},
    {NULL, NULL, NULL}
};

int main(void)
{
    _init_tlayer();
    thread_create(_rcv_stack_buf, KERNEL_CONF_STACKSIZE_MAIN, PRIORITY_MAIN, CREATE_STACKTEST, _demo_receiver_thread, "_demo_receiver_thread");

    // start shell
    posix_open(uart0_handler_pid, 0);

    printf("\n\t\t\tWelcome to RIOT\n\n");

    shell_t shell;
    shell_init(&shell, shell_commands, UART0_BUFSIZE, uart0_readc, uart0_putc);

    shell_run(&shell);

    return 0;
}
