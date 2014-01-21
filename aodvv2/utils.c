#include "utils.h"

#define ENABLE_DEBUG (1)
#include "debug.h"

/* Some aodvv2 utilities (mostly tables) */

/* helper functions */
static struct aodvv2_rreq_entry* get_comparable_rreq(struct aodvv2_packet_data* packet_data);
static void add_rreq(struct aodvv2_packet_data* packet_data);
static void _reset_entry_if_stale(uint8_t i);

static struct aodvv2_client_addresses client_table[AODVV2_MAX_CLIENTS];
static struct aodvv2_rreq_entry rreq_table[AODVV2_RREQ_BUF];

struct netaddr_str nbuf;
timex_t null_time, now, expiration_time;

/*
 * Initialize table of clients that the router currently serves.
 */
void init_clienttable(void)
{   
    for (uint8_t i = 0; i < AODVV2_MAX_CLIENTS; i++) {
        memset(&client_table[i], 0, sizeof(client_table[i]));
    }
    /* Add myself to clienttable. TODO: use proper IP!!! */
    struct netaddr my_ip;
    netaddr_from_string(&my_ip, MY_IP);    
    add_client(&my_ip, 0);

    DEBUG("[aodvv2] client table initialized.\n");
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
    null_time = timex_set(0,0);

    for (uint8_t i = 0; i < AODVV2_RREQ_BUF; i++) {
        memset(&rreq_table[i], 0, sizeof(rreq_table[i]));
    }
    DEBUG("[aodvv2] RREQ table initialized.\n");
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
    int seqNum_comparison;
    timex_t now;
    
    comparable_rreq = get_comparable_rreq(packet_data);
    
    /* if there is no comparable rreq stored, add one and return false */
    if (comparable_rreq == NULL){
        add_rreq(packet_data);
        return false;
    }

    seqNum_comparison = cmp_seqnum(packet_data->origNode.seqNum, comparable_rreq->seqNum);

    /* 
     * If two RREQs have the same
     * metric type and OrigNode and Targnode addresses, the information from
     * the one with the older Sequence Number is not needed in the table
     */
    if (seqNum_comparison == -1)
        return true;

    if (seqNum_comparison == 1)
        /* Update RREQ table entry with new seqnum value */
        comparable_rreq->seqNum = packet_data->origNode.seqNum;

    /* 
     * in case they have the same Sequence Number, the one with the greater
     * Metric value is not needed
     */
    if (seqNum_comparison == 0){
        if (comparable_rreq->metric <= packet_data->origNode.metric)
            return true;
        /* Update RREQ table entry with new metric value */
        comparable_rreq->metric = packet_data->origNode.metric;
    }

    /* Since we've changed RREQ info, update the timestamp */
    rtc_time(&now);
    comparable_rreq->timestamp = now;
    return false;

}

/*
 * retrieve pointer to a comparable (according to Section 6.7.) 
 * RREQ table entry. To edit, simply follow the pointer.
 * Two AODVv2 RREQ messages are comparable if:

   o  they have the same metric type
   o  they have the same OrigNode and TargNode addresses
   if there is no comparable RREQ, return NULL.
 */
static struct aodvv2_rreq_entry* get_comparable_rreq(struct aodvv2_packet_data* packet_data)
{       
    for (uint8_t i = 0; i < AODVV2_RREQ_BUF; i++) {
        
        _reset_entry_if_stale(i);

        if (!netaddr_cmp(&rreq_table[i].origNode, &packet_data->origNode.addr)
            && !netaddr_cmp(&rreq_table[i].targNode, &packet_data->targNode.addr)
            && rreq_table[i].metricType == packet_data->metricType){
            return &rreq_table[i];          
        }
    }

    return NULL;
}

static void add_rreq(struct aodvv2_packet_data* packet_data)
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

/* 
 * Check if entry at index i is stale and clear the space it takes up if it is
 * (because we've implemented our table crappily) 
 */
static void _reset_entry_if_stale(uint8_t i)
{
    rtc_time(&now);
    expiration_time = timex_sub(now, timex_set(AODVV2_MAX_IDLETIME, 0));

    if (timex_cmp(rreq_table[i].timestamp, null_time) != 0){
        if (timex_cmp(rreq_table[i].timestamp, expiration_time) < 0){
            DEBUG("\treset rreq table entry %s\n", netaddr_to_string(&nbuf, &rreq_table[i].origNode));

            memset(&rreq_table[i], 0, sizeof(rreq_table[i]));
        }
    }
}