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

#include "vtimer.h"
#include "rtc.h"
#include "board_uart0.h"
#include "shell.h"
#include "shell_commands.h"
#include "board.h"
#include "posix_io.h"
#include "nativenet.h"
#include "msg.h"
#include <thread.h>

#include "common/common_types.h"
#include "common/netaddr.h"

#include "rfc5444/rfc5444_reader.h"
#include "rfc5444/rfc5444_writer.h"
#include "rfc5444/rfc5444_print.h"

#include "aodvv2_reader.h"
#include "aodvv2_writer.h"
#include "sender.h"
#include "destiny.h"
#include "destiny/socket.h"

#define ENABLE_DEBUG (1)
#include "debug.h"

#include "include/aodvv2.h"

/*
const shell_command_t shell_commands[] = {
    {"rreq", "send rreq", send_rreq},
    {"rrep", "send rrep", send_rrep},
    {"receive_udp", "receive udp packets", receive_udp},
};
*/

int main(int argc __attribute__ ((unused)), char **argv __attribute__ ((unused)))
{
    shell_t shell;
    struct tm localt;

    /* init aodv stuff */
    aodv_init();

    posix_open(uart0_handler_pid, 0);

    printf("\n\t\t\tWelcome to RIOT\n\n");

    rtc_get_localtime(&localt);
    printf("The time is now: %s\n", asctime(&localt));

    /* fancy greeting */
    printf("Hold on half a second...\n");
    LED_RED_TOGGLE;
    vtimer_usleep(500000);
    LED_RED_TOGGLE;
    LED_GREEN_ON;
    LED_GREEN_OFF;

    printf("\n\t\t\tWelcome to RIOT\n\n");

    printf("You may use the shell now.\n");

    /*
    shell_init(&shell, shell_commands, uart0_readc, uart0_putc);
    shell_run(&shell);
    */
    //test_tables_main();
    test_packets_main();

    struct netaddr address, origNode, targNode;
    netaddr_from_string(&address, "::12");
    netaddr_from_string(&targNode, "::13");
    netaddr_from_string(&origNode, "::14");
    struct aodvv2_packet_data entry_1 = {
        .hoplimit = AODVV2_MAX_HOPCOUNT,
        .sender = address,
        .metricType = AODVV2_DEFAULT_METRIC_TYPE,
        .origNode = {
            .addr = origNode,
            .prefixlen = 128,
            .metric = 12,
            .seqNum = 13,
        },
        .targNode = {
            .addr = targNode,
            .prefixlen = 128,
            .metric = 12,
            .seqNum = 0,
        },
        .timestamp = NULL,
    };

    //send_rrep(&entry_1, &targNode);
    //send_rreq("");
    
    return 0;
}
