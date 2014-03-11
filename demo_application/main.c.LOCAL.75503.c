#include <stdio.h>
#include <string.h>
#include <unistd.h> // for getting the pid
#include <inet_pton.h>

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

#define RANDOM_PORT 1337
#define UDP_BUFFER_SIZE     (128)
#define RCV_MSG_Q_SIZE      (64)

static int _sock_snd, if_id;
static sockaddr6_t _sockaddr;
static ipv6_addr_t prefix;

msg_t msg_q[RCV_MSG_Q_SIZE];

char _rcv_stack_buf[KERNEL_CONF_STACKSIZE_MAIN];

void demo_send(char* id_str) // hier beschwert er sich: demo_send (id_str=0x3 <Address 0x3 out of bounds>) 
{
    char dest_str[IPV6_MAX_ADDR_STR_LEN];  // assuming we're dealing with "full" IPs
    char msg[25];

    int res = sscanf(id_str, "send %s %s", dest_str, msg);

    if (res != 2) {
        printf("Usage: send <destination ip> <message>\n");
        return;
    }
    printf("sending...\n");

    // turn dest_str into ipv6_addr_t
    inet_pton(AF_INET6, dest_str, &_sockaddr.sin6_addr);

    int bytes_sent = destiny_socket_sendto(_sock_snd, msg, strlen(msg)+1, 
                                            0, &_sockaddr, sizeof _sockaddr);

    if (bytes_sent == -1)
        printf("no bytes sent, probably because there is no route yet.\n");
    else
        printf("%d bytes sent.\n", bytes_sent);
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
    //destiny_init_transport_layer();
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
    {"ip", "Print all addresses attached to this device", demo_print_ip},
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
