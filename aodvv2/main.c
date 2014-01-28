#include <string.h>
#include <stdio.h>
#include <time.h>
#include <thread.h>

#include <sys/types.h>
#include <unistd.h>

#include "vtimer.h"
#include "board_uart0.h"
#include "shell.h"
#include "shell_commands.h"
#include "board.h"
#include "posix_io.h"
#include "nativenet.h"
#include "msg.h"
#include "destiny.h"
#include "destiny/socket.h"

#include "common/common_types.h"
#include "common/netaddr.h"

#include "rfc5444/rfc5444_reader.h"
#include "rfc5444/rfc5444_writer.h"
#include "rfc5444/rfc5444_print.h"

#include "reader.h"
#include "writer.h"
#include "aodv.h"

#define ENABLE_DEBUG (1)
#include "debug.h"

#include "include/aodvv2.h"

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


int main(int argc __attribute__ ((unused)), char **argv __attribute__ ((unused)))
{
    shell_t shell;
    struct tm localt;

    /* init aodv stuff */
    init_tlayer();
    aodv_init();

    posix_open(uart0_handler_pid, 0);

    printf("\n\t\t\tWelcome to RIOT\n\n");

    /* fancy greeting */
    printf("Hold on half a second...\n");
    LED_RED_TOGGLE;
    vtimer_usleep(500000);
    LED_RED_TOGGLE;
    LED_GREEN_ON;
    LED_GREEN_OFF;

    printf("\n\t\t\tWelcome to RIOT\n\n");

    /*
    printf("You may use the shell now.\n");

    shell_init(&shell, shell_commands, UART0_BUFSIZE, uart0_readc, uart0_putc);
    shell_run(&shell);
    */
    //test_tables_main();
    //test_packets_main();

    return 0;
}
