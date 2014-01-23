/*
 * Copyright (C) 2013 Ludwig Ortmann <ludwig.ortmann@fu-berlin.de>
 */


/*
 * The olsr.org Optimized Link-State Routing daemon version 2 (olsrd2)
 * Copyright (c) 2004-2013, the olsr.org team - see HISTORY file
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * * Redistributions of source code must retain the above copyright
 *   notice, this list of conditions and the following disclaimer.
 * * Redistributions in binary form must reproduce the above copyright
 *   notice, this list of conditions and the following disclaimer in
 *   the documentation and/or other materials provided with the
 *   distribution.
 * * Neither the name of olsr.org, olsrd nor the names of its
 *   contributors may be used to endorse or promote products derived
 *   from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 * COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 * ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 * Visit http://www.olsr.org for more information.
 *
 * If you find this software useful feel free to make a donation
 * to the project. For more information see the website or contact
 * the copyright holders.
 *
 */

#include <string.h>
#include <stdio.h>
#include <time.h>
#include <thread.h>

#include <sys/types.h>
#include <unistd.h>

#include "vtimer.h"
//#include "rtc.h"
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

    vtimer_get_localtime(&localt);
    printf("The time is now: %s\n", asctime(&localt));

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
    test_tables_main();
    //test_packets_main();

    return 0;
}
