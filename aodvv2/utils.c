#include "utils.h"

#define ENABLE_DEBUG (1)
#include "debug.h"

/* Some aodvv2 utilities (mostly tables) */
static mutex_t clientt_mutex;
static mutex_t rreqt_mutex;

/* helper functions */
static struct aodvv2_rreq_entry* _get_comparable_rreq(struct aodvv2_packet_data* packet_data);
static void _add_rreq(struct aodvv2_packet_data* packet_data);
static void _reset_entry_if_stale(uint8_t i);

static struct netaddr client_table[AODVV2_MAX_CLIENTS];
static struct aodvv2_rreq_entry rreq_table[AODVV2_RREQ_BUF];

static struct netaddr_str nbuf;
static timex_t null_time, now, _max_idletime;

/*
 * Initialize table of clients that the router currently serves.
 */
void clienttable_init(void)
{   
    if (mutex_lock(&clientt_mutex) == 1) {
        for (uint8_t i = 0; i < AODVV2_MAX_CLIENTS; i++) {
            memset(&client_table[i], 0, sizeof(client_table[i]));
        }
        mutex_unlock(&clientt_mutex);
    }

    DEBUG("[aodvv2] client table initialized.\n");
}

/*
 * Add client to the list of clients that the router currently serves. 
 * Since the current version doesn't offer support for Client Networks,
 * the prefixlen is currently ignored.
 */
void clienttable_add_client(struct netaddr* addr)
{
    if(!clienttable_is_client(addr)) {
        /*find free spot in client table and place client address there */
        if (mutex_lock(&clientt_mutex) == 1) {
            for (uint8_t i = 0; i < AODVV2_MAX_CLIENTS; i++){
                if (client_table[i]._type == AF_UNSPEC
                    && client_table[i]._prefix_len == 0) {
                    client_table[i] = *addr;
                    DEBUG("[aodvv2] clienttable: added client %s\n", netaddr_to_string(&nbuf, addr));
                    mutex_unlock(&clientt_mutex);
                    return;
                }
            }
        }
    }
}

/*
 * Find out if a client is in the list of clients that the router currently serves. 
 * Since the current version doesn't offer support for Client Networks,
 * the prefixlen embedded in the netaddr is currently ignored.
 */
bool clienttable_is_client(struct netaddr* addr)
{
    if (mutex_lock(&clientt_mutex) == 1) {
        for (uint8_t i = 0; i < AODVV2_MAX_CLIENTS; i++) {
            if (!netaddr_cmp(&client_table[i], addr)) {
                mutex_unlock(&clientt_mutex);
                return true;
            }
        }
        mutex_unlock(&clientt_mutex);
    }
    return false;
}

/*
 * Delete a client from the list of clients that the router currently serves. 
 * Since the current version doesn't offer support for Client Networks,
 * the prefixlen embedded in the netaddr is currently ignored.
 */
void clienttable_delete_client(struct netaddr* addr)
{
    if (!clienttable_is_client(addr))
        return;
    
    if (mutex_lock(&clientt_mutex) == 1) {
        for (uint8_t i = 0; i < AODVV2_MAX_CLIENTS; i++) {
            if (!netaddr_cmp(&client_table[i], addr)) {
                memset(&client_table[i], 0, sizeof(client_table[i]));
                mutex_unlock(&clientt_mutex);
                return;
            }
        }
    }
}

/*
 * Initialize table of clients that the router currently serves.
 */
void rreqtable_init(void)
{
    if (mutex_lock(&rreqt_mutex) == 1) {
        null_time = timex_set(0,0);
        _max_idletime = timex_set(AODVV2_MAX_IDLETIME, 0);

        for (uint8_t i = 0; i < AODVV2_RREQ_BUF; i++) {
            memset(&rreq_table[i], 0, sizeof(rreq_table[i]));
        }
        mutex_unlock(&rreqt_mutex);
    }
    DEBUG("[aodvv2] RREQ table initialized.\n");
}

/**
 * Check if a RREQ is redundant, i.e. was achieved form another node already.
 * 
 * Behaves as described in Sections 5.7. and 7.6.:
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
 * @param packet_data
 */
bool rreqtable_is_redundant(struct aodvv2_packet_data* packet_data)
{
    struct aodvv2_rreq_entry* comparable_rreq;
    int seqnum_comparison;
    timex_t now;
    
    if (mutex_lock(&rreqt_mutex) == 1) {
        comparable_rreq = _get_comparable_rreq(packet_data);
        
        /* if there is no comparable rreq stored, add one and return false */
        if (comparable_rreq == NULL){
            _add_rreq(packet_data);
            mutex_unlock(&rreqt_mutex);
            return false;
        }

        seqnum_comparison = seqnum_cmp(packet_data->origNode.seqnum, comparable_rreq->seqnum);

        /* 
         * If two RREQs have the same
         * metric type and OrigNode and Targnode addresses, the information from
         * the one with the older Sequence Number is not needed in the table
         */
        if (seqnum_comparison == -1){
            mutex_unlock(&rreqt_mutex);
            return true;
        }

        if (seqnum_comparison == 1){
            /* Update RREQ table entry with new seqnum value */
            comparable_rreq->seqnum = packet_data->origNode.seqnum;
        }

        /* 
         * in case they have the same Sequence Number, the one with the greater
         * Metric value is not needed
         */
        if (seqnum_comparison == 0){
            if (comparable_rreq->metric <= packet_data->origNode.metric){
                mutex_unlock(&rreqt_mutex);
                return true;
            }
            /* Update RREQ table entry with new metric value */
            comparable_rreq->metric = packet_data->origNode.metric;
        }

        /* Since we've changed RREQ info, update the timestamp */
        vtimer_now(&now);
        comparable_rreq->timestamp = now;
        mutex_unlock(&rreqt_mutex);
    }

    return false;
}

/*
 * retrieve pointer to a comparable (according to Section 6.7.) 
 * RREQ table entry if it exists and NULL otherwise.
 * Two AODVv2 RREQ messages are comparable if:
 * - they have the same metric type
 * - they have the same OrigNode and TargNode addresses
 */
static struct aodvv2_rreq_entry* _get_comparable_rreq(struct aodvv2_packet_data* packet_data)
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


static void _add_rreq(struct aodvv2_packet_data* packet_data)
{
    if (!(_get_comparable_rreq(packet_data))){
        /*find empty rreq and fill it with packet_data */

        for (uint8_t i = 0; i < AODVV2_RREQ_BUF; i++){
            if (!rreq_table[i].timestamp.seconds
                && !rreq_table[i].timestamp.microseconds) {
                /* TODO: sanity check? */
                rreq_table[i].origNode = packet_data->origNode.addr; 
                rreq_table[i].targNode = packet_data->targNode.addr; 
                rreq_table[i].metricType = packet_data->metricType; 
                rreq_table[i].metric = packet_data->origNode.metric; 
                rreq_table[i].seqnum = packet_data->origNode.seqnum;
                rreq_table[i].timestamp = packet_data->timestamp;
                return;
            }
        }
    }    
}

/* 
 * Check if entry at index i is stale and clear the struct it fills if it is
 */
static void _reset_entry_if_stale(uint8_t i)
{
    vtimer_now(&now);

    if (timex_cmp(rreq_table[i].timestamp, null_time) != 0){
        timex_t expiration_time = timex_add(rreq_table[i].timestamp, _max_idletime);
        if (timex_cmp(expiration_time, now) < 0){
            /* timestamp+expiration time is in the past: this entry is stale */
            DEBUG("\treset rreq table entry %s\n", netaddr_to_string(&nbuf, &rreq_table[i].origNode));
            memset(&rreq_table[i], 0, sizeof(rreq_table[i]));
        }
    }
}

void ipv6_addr_t_to_netaddr(ipv6_addr_t* src, struct netaddr* dst)
{
    dst->_type = AF_INET6;
    dst->_prefix_len = AODVV2_RIOT_PREFIXLEN;
    memcpy(dst->_addr, src, sizeof dst->_addr);
}

void netaddr_to_ipv6_addr_t(struct netaddr* src, ipv6_addr_t* dst)
{
    for (int i=0; i<NETADDR_MAX_LENGTH; i++){
        memcpy(dst, src->_addr, sizeof(uint8_t) * NETADDR_MAX_LENGTH);
    }
}
