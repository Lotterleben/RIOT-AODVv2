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

const shell_command_t shell_commands[] = {
    {NULL, NULL, NULL}
};

//#if defined(BOARD_NATIVE)
//#include <unistd.h>
static uint8_t transceiver_type = TRANSCEIVER_NATIVE;

/* gemopst von ben */
static uint16_t get_node_id(void) {
    DEBUG("node id is: %d \n", getpid());
    return getpid();
}
//#endif

/*
const shell_command_t shell_commands[] = {
    {"rreq", "send rreq", old_send_rreq},
    {"rrep", "send rrep", old_send_rrep},
    {"snd", "send message", aodv_send},
    {"rcv", "receive udp packets", aodv_receive},
    {NULL, NULL, NULL}
};
*/

/* init transport layer stuff */
void init_tlayer(void)
{
    DEBUG("node id is: %d \n", getpid());
    //destiny_init_transport_layer();
    sixlowpan_lowpan_init(transceiver_type, get_node_id(), 0);
}

int main(void)
{
    init_tlayer();
    aodv_init();

    /* start shell */
    posix_open(uart0_handler_pid, 0);

    printf("\n\t\t\tWelcome to RIOT\n\n");

    shell_t shell;
    shell_init(&shell, shell_commands, UART0_BUFSIZE, uart0_readc, uart0_putc);

    shell_run(&shell);
    return 0;
}