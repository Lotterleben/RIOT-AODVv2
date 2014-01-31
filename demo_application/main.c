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

#include "include/aodvv2.h"

#define ENABLE_DEBUG (1)
#include "debug.h"

#define RANDOM_PORT 1337

//#if defined(BOARD_NATIVE)
//#include <unistd.h>
static uint8_t transceiver_type = TRANSCEIVER_NATIVE;
//#endif

static int _sock_snd;
static sockaddr6_t _sockaddr;


void demo_send(char *id_str)
{
    char dest_str[39];  // assuming we're dealing with "full" IPs
    char msg[25];
    ipv6_addr_t dest_ip;

    int res = sscanf(id_str, "send %s %s", dest_str, msg);

    if (res != 2) {
        printf("Usage: send <destination ip> <message>\n");
        return;
    }
    printf("sending...\n");

    // turn dest_ip into ipv6_addr_t
    inet_pton(AF_INET6, dest_str, &_sockaddr.sin6_addr);

    int bytes_sent = destiny_socket_sendto(_sock_snd, msg, strlen(msg)+1, 
                                            0, &_sockaddr, sizeof _sockaddr);

    printf("%d bytes sent.\n", bytes_sent);
}

void demo_init_socket(void)
{
    _sockaddr.sin6_family = AF_INET6;
    _sockaddr.sin6_port = HTONS(RANDOM_PORT);

    _sock_snd = destiny_socket(PF_INET6, SOCK_DGRAM, IPPROTO_UDP);

    if(-1 == _sock_snd) {
        printf(" Error Creating Socket!");
        return;
    }
}

/* init transport layer & routing stuff*/
void init_tlayer(char *str)
{
    //destiny_init_transport_layer();
    printf("initializing 6LoWPAN...\n");
    sixlowpan_lowpan_init(transceiver_type, getpid(), 0);
    printf("initializing AODVv2...\n");
    aodv_init();
    demo_init_socket();
}

const shell_command_t shell_commands[] = {
    {"send", "send message to ip", demo_send},
    {"init", "init transport layer and aodv", init_tlayer},
    {NULL, NULL, NULL}
};

int main(void)
{
    test_tables_main();

    /*
    init_tlayer("");

    // start shell
    posix_open(uart0_handler_pid, 0);

    printf("\n\t\t\tWelcome to RIOT\n\n");

    shell_t shell;
    shell_init(&shell, shell_commands, UART0_BUFSIZE, uart0_readc, uart0_putc);

    shell_run(&shell);
    */

    return 0;
}











