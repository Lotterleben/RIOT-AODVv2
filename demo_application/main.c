#include <stdio.h>
#include <string.h>
#include <unistd.h> // for getting the pid

#include "thread.h"
#include "posix_io.h"
#include "shell.h"
#include "shell_commands.h"
#include "board_uart0.h"
#include "destiny.h"

#include "kernel.h"

#include "include/aodvv2.h"

#define ENABLE_DEBUG (1)
#include "debug.h"

//#if defined(BOARD_NATIVE)
//#include <unistd.h>
static uint8_t transceiver_type = TRANSCEIVER_NATIVE;
static uint16_t get_node_id(void) {
    return getpid();
}
//#endif

void demo_send(char *id_str)
{
    char* dest_ip, msg;

    int res = sscanf(id_str, "send %s %s", &dest_ip, &msg);

    printf("res: %i\n", res); 
    if (res != 2) {
        printf("Usage: send <destination ip> <message>\n");
        return;
    }

    // turn dest_ip into ipv6_addr_t
    // todo: check if valid IP
    printf("TODO: init socket & send!\n");
}

/* init transport layer & routing stuff*/
void init_tlayer(char *str)
{
    //destiny_init_transport_layer();
    printf("initializing 6LoWPAN...\n");
    sixlowpan_lowpan_init(transceiver_type, getpid(), 0);
    printf("initializing AODVv2...\n");
    aodv_init();
}

const shell_command_t shell_commands[] = {
    {"send", "send message to ip", demo_send},
    {"init", "init transport layer and aodv", init_tlayer},
    {NULL, NULL, NULL}
};

int main(void)
{
    init_tlayer("");

    /* start shell */
    posix_open(uart0_handler_pid, 0);

    printf("\n\t\t\tWelcome to RIOT\n\n");

    shell_t shell;
    shell_init(&shell, shell_commands, UART0_BUFSIZE, uart0_readc, uart0_putc);

    shell_run(&shell);
    return 0;
}
