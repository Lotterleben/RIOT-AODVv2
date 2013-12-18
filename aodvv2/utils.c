/* Some aodvv2 utilities (mostly tables) */
#include <stdio.h>
#include "utils.h"
#include "include/aodvv2.h"

static struct aodvv2_client_addresses client_table[AODVV2_MAX_CLIENTS];
static struct aodvv2_rreq_entry rreq_table[AODVV2_RREQ_BUF];

/* helper functions */
struct aodvv2_rreq_entry* get_comparable_rreq(struct aodvv2_packet_data* packet_data);
void add_rreq(struct aodvv2_packet_data* packet_data);

/*
 * Initialize table of clients that the router currently serves.
 */
void init_clienttable(void)
{   
    for (uint8_t i = 0; i < AODVV2_MAX_CLIENTS; i++) {
        memset(&client_table[i], 0, sizeof(client_table[i]));
    }
    printf("[aodvv2] client table initialized.\n");
}

/*
 * Add client to the list of clients that the router currently serves. 
 * Since the current version doesn't offer support for Client Networks,
 * the prefixlen is currently ignored.
 */
void add_client(struct netaddr* addr, uint8_t prefixlen)
{
    if(!is_client(addr, prefixlen)){
        /*find free spot in client table and place client address there */
        for (uint8_t i = 0; i < AODVV2_MAX_CLIENTS; i++){
            if (client_table[i].address._type == AF_UNSPEC
                && client_table[i].prefixlen == 0) {
                client_table[i].address = *addr;
                client_table[i].prefixlen = prefixlen; 
                return;
            }
        }
    }
}

/*
 * Find out if a client is in the list of clients that the router currently serves. 
 * Since the current version doesn't offer support for Client Networks,
 * the prefixlen is currently ignored.
 */
bool is_client(struct netaddr* addr, uint8_t prefixlen)
{
    for (uint8_t i = 0; i < AODVV2_MAX_CLIENTS; i++) {
        if (!netaddr_cmp(&client_table[i].address, addr))
            return true;
    }
    return false;
}

/*
 * Delete a client from the list of clients that the router currently serves. 
 * Since the current version doesn't offer support for Client Networks,
 * the prefixlen is currently ignored.
 */
void delete_client(struct netaddr* addr, uint8_t prefixlen)
{
    if (!is_client(addr, prefixlen))
        return;
    
    for (uint8_t i = 0; i < AODVV2_MAX_CLIENTS; i++) {
        if (!netaddr_cmp(&client_table[i].address, addr)) {
            memset(&client_table[i], 0, sizeof(client_table[i]));
            return;
        }
    }
}

/*
 * Initialize table of clients that the router currently serves.
 */
void init_rreqtable(void)
{
    for (uint8_t i = 0; i < AODVV2_RREQ_BUF; i++) {
        memset(&rreq_table[i], 0, sizeof(rreq_table[i]));
    }
    printf("[aodvv2] RREQ table initialized.\n");
}

/*
 * Behaviour described in Sections 5.7. and 7.6.:
 *
 * The RREQ Table is maintained so that no two entries in the RREQ Table
 * are comparable -- that is, all RREQs represented in the RREQ Table
 * either have different OrigNode addresses, different TargNode
 * addresses, or different metric types.  If two RREQs have the same
 * metric type and OrigNode and Targnode addresses, the information from
 * the one with the older Sequence Number is not needed in the table; in
 * case they have the same Sequence Number, the one with the greater
 * Metric value is not needed; in case they have the same Metric as
 * well, it does not matter which table entry is maintained.  Whenever a
 * RREQ Table entry is updated, its Timestamp field should also be
 * updated to reflect the Current_Time.
 *
 * 
 */
bool rreq_is_redundant(struct aodvv2_packet_data* packet_data)
{
    struct aodvv2_rreq_entry* comparable_rreq;

    /* if there is no comparable rreq stored, add one and  */
    if (!(comparable_rreq = get_comparable_rreq(packet_data))){
        add_rreq(packet_data);
        return false;
    }

    // TODO
}

/*
 * retrieve pointer to a comparable (according to Section 6.7.) 
 * RREQ table entry. To edit, simply follow the pointer.
 */
struct aodvv2_rreq_entry* get_comparable_rreq(struct aodvv2_packet_data* packet_data)
{   
    timex_t now, expiration_time;
    rtc_time(&now);
    expiration_time = timex_sub(now, timex_set(AODVV2_MAX_IDLETIME, 0));
    
    for (uint8_t i = 0; i < AODVV2_RREQ_BUF; i++) {
        // TODO: first, check if timestamp stale & memset in case
        //if (timex_cmp(rreq_table[i])){

        //}
        if (!netaddr_cmp(&rreq_table[i].origNode, &packet_data->origNode.addr)
            && !netaddr_cmp(&rreq_table[i].targNode, &packet_data->targNode.addr)
            && rreq_table[i].metricType == packet_data->metricType) {
            return &rreq_table[i];          
        }
    }
    return NULL;
}

void add_rreq(struct aodvv2_packet_data* packet_data)
{
    if (!(get_comparable_rreq(packet_data))){
        /*find empty rreq and fill it with packet_data */
        timex_t now;

        for (uint8_t i = 0; i < AODVV2_RREQ_BUF; i++){
            if (!rreq_table[i].timestamp.seconds
                && !rreq_table[i].timestamp.microseconds) {
                /* TODO: sanity check? */
                rreq_table[i].origNode = packet_data->origNode.addr; 
                rreq_table[i].targNode = packet_data->targNode.addr; 
                rreq_table[i].metricType = packet_data->metricType; 
                rreq_table[i].metric = packet_data->origNode.metric; 
                rreq_table[i].seqNum = packet_data->origNode.seqNum;
                rreq_table[i].timestamp = packet_data->timestamp;
                return;
            }
        }
    }    
}








