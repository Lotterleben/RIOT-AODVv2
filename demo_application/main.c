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

radio_address_t _id;

//#if defined(BOARD_NATIVE)
//#include <unistd.h>
static uint8_t transceiver_type = TRANSCEIVER_NATIVE;

/* gemopst von ben */
static uint16_t get_node_id(void) {
    DEBUG("node id is: %d \n", getpid());
    return getpid();
}
//#endif

void demo_set_id(char *id_str)
{
    int res = sscanf(id_str, "set %hu", &_id);

    if (res < 1) {
        printf("Usage: init address\n");
        printf("\taddress must be an 8 bit integer\n");
        printf("\n\t(Current ID is %u)\n", _id);
        return;
    }

    printf("Set node ID to %u\n", _id);
}

/* init transport layer stuff and aodv*/
void init_tlayer(char *str)
{
    if (!_id) {
        printf("Error: set node id with 'set' command first!\n");
        return;
    }

    //destiny_init_transport_layer();
    printf("\tinitializing sixlowpan...\n");
    sixlowpan_lowpan_init(transceiver_type, get_node_id(), 0);
    printf("\tinitializing aodv...\n");
    aodv_init();
}

const shell_command_t shell_commands[] = {
    {"set", "set node ID", demo_set_id},
    {"init", "init transport layer and aodv", init_tlayer},
    {NULL, NULL, NULL}
};

int main(void)
{
    /* start shell */
    posix_open(uart0_handler_pid, 0);

    printf("\n\t\t\tWelcome to RIOT\n\n");

    shell_t shell;
    shell_init(&shell, shell_commands, UART0_BUFSIZE, uart0_readc, uart0_putc);

    shell_run(&shell);
    return 0;
}