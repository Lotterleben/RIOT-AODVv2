/*
 * Copyright (C) 2015
 *
 * This file is subject to the terms and conditions of the GNU Lesser
 * General Public License v2.1. See the file LICENSE in the top level
 * directory for more details.
 */

/**
 * @ingroup     tests
 * @{
 *
 * @file
 * @brief       AODVv2 udp application
 *
 * @author
 *
 * @}
 */

#include <stdio.h>
#include <unistd.h>

#include "embUnit/embUnit.h"

#include "udp.h"
#include "thread.h"

#include "aodvv2/aodvv2.h"

#include "aodv_writer_tests.h"
#include "aodv_tests.h"

#define AODVV2_CHANNEL         (26)     /**< The used channel */
#define AODVV2_PAN             (0x03e9) /**< The used PAN ID */
#define AODVV2_IFACE           (0)      /**< The used Trasmssion device */
#define AODVV2_UDP_PORT        (12345)  /**< The UDP port to listen */
#define AODVV2_UDP_BUFFER_SIZE (1024)   /**< The buffer size for receiving UDPs */

/** The UDP server thread stack */
char udp_server_stack_buffer[KERNEL_CONF_STACKSIZE_MAIN];
/** The node IPv6 address */
ipv6_addr_t myaddr;

radio_address_t aodvv2_iface_id;
struct netaddr aodvv2_test_sender_oa, aodvv2_test_sender_ta, aodvv2_test_origaddr, aodvv2_test_targaddr;

/**
* @brief prepares this node
* @return 0 on success
*/
static int aodvv2_setup_node(void)
{
    /* setup the radio interface */
    net_if_set_src_address_mode(AODVV2_IFACE, NET_IF_TRANS_ADDR_M_SHORT);
    aodvv2_iface_id = net_if_get_hardware_address(AODVV2_IFACE);

    /* choose addresses */
    ipv6_addr_init(&myaddr, 0x2015, 0x3, 0x18, 0x1111, 0x0, 0x0, 0x0, aodvv2_iface_id);

    /* and set it */
    ipv6_net_if_add_addr(AODVV2_IFACE, &myaddr, NDP_ADDR_STATE_PREFERRED, 0, 0, 0);

    return 0;
}

/**
* @brief init data that needs to be globally known
*/
static void aodvv2_init_testdata(void)
{
    int i; /* make compiler stfu */
    i = netaddr_from_string(&aodvv2_test_origaddr, "::10");
    i = netaddr_from_string(&aodvv2_test_sender_oa, "::11");
    i = netaddr_from_string(&aodvv2_test_sender_ta, "::12");
    i = netaddr_from_string(&aodvv2_test_targaddr, "::13");
}

int main(void)
{
    aodvv2_init_testdata();
    aodvv2_setup_node();
    aodv_init();

    write_packets_to_files();

    sleep(5);

    return 0;
}
