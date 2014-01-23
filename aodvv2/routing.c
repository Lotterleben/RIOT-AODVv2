/*
 * Cobbled-together routing table.
 * This is neither efficient nor elegant, but until RIOT gets their own native
 * RT, this will have to do.
 */

#include "routing.h" 

#define ENABLE_DEBUG (1)
#include "debug.h"

/* helper functions */
static void _reset_entry_if_stale(uint8_t i);

static struct aodvv2_routing_entry_t routing_table[AODVV2_MAX_ROUTING_ENTRIES];
timex_t now, null_time;
struct netaddr_str nbuf;

void init_routingtable(void)
{   
    null_time = timex_set(0,0);

    for (uint8_t i = 0; i < AODVV2_MAX_ROUTING_ENTRIES; i++) {
        memset(&routing_table[i], 0, sizeof(routing_table[i]));
    }
    DEBUG("[aodvv2] routing table initialized.\n");
}

struct netaddr* routingtable_get_next_hop(struct netaddr* addr, uint8_t metricType)
{
    struct aodvv2_routing_entry_t* entry = routingtable_get_entry(addr, metricType);
    if (!entry)
        return NULL;
    return(&entry->nextHopAddress);
}


void routingtable_add_entry(struct aodvv2_routing_entry_t* entry)
{
    /* only update if we don't already know the address
     * TODO: does this always make sense?
     */
    if (!(routingtable_get_entry(&(entry->address), entry->metricType))){
        /*find free spot in RT and place rt_entry there */
        for (uint8_t i = 0; i < AODVV2_MAX_ROUTING_ENTRIES; i++){
            if (routing_table[i].address._type == AF_UNSPEC) {
                /* TODO: sanity check? */
                memcpy(&routing_table[i], entry, sizeof(struct aodvv2_routing_entry_t));
                return;
            }
        }
    }
}

/*
 * retrieve pointer to a routing table entry. To edit, simply follow the pointer.
 */
struct aodvv2_routing_entry_t* routingtable_get_entry(struct netaddr* addr, uint8_t metricType)
{   
    for (uint8_t i = 0; i < AODVV2_MAX_ROUTING_ENTRIES; i++) {
        _reset_entry_if_stale(i);

        if (!netaddr_cmp(&routing_table[i].address, addr)
            && routing_table[i].metricType == metricType) {
            return &routing_table[i];
        }
    }
    return NULL;
}

void routingtable_delete_entry(struct netaddr* addr, uint8_t metricType)
{
    for (uint8_t i = 0; i < AODVV2_MAX_ROUTING_ENTRIES; i++) {
        _reset_entry_if_stale(i);

        if (!netaddr_cmp(&routing_table[i].address, addr)
            && routing_table[i].metricType == metricType) {
            memset(&routing_table[i], 0, sizeof(routing_table[i]));
            return;
        }
    }
}

/* 
 * Check if entry at index i is stale and clear the space it takes up if it is
 * (because we've implemented our table crappily) 
 */
static void _reset_entry_if_stale(uint8_t i)
{
    vtimer_now(&now);

    if (timex_cmp(routing_table[i].expirationTime, null_time) != 0){
        if(timex_cmp(routing_table[i].expirationTime, now) < 1){
            DEBUG("\treset routing table entry for %s at %i\n", netaddr_to_string(&nbuf, &routing_table[i].address), i);
            memset(&routing_table[i], 0, sizeof(routing_table[i]));
        }
    }
}

void print_routingtable(void)
{   
    struct aodvv2_routing_entry_t curr_entry;

    printf("===== BEGIN ROUTING TABLE ===================\n");
    for(int i = 0; i < AODVV2_MAX_ROUTING_ENTRIES; i++) {
        // route has been used before => non-empty entry
        if (routing_table[i].lastUsed.seconds || routing_table[i].lastUsed.microseconds) {
            print_routingtable_entry(&routing_table[i]);
        }
    }
    printf("===== END ROUTING TABLE =====================\n");
}

void print_routingtable_entry(struct aodvv2_routing_entry_t* rt_entry)
{   
    struct netaddr_str nbuf;

    printf(".................................\n");
    printf("\t address: %s\n", netaddr_to_string(&nbuf, &(rt_entry->address))); 
    printf("\t prefixlen: %i\n", rt_entry->prefixlen);
    printf("\t seqNum: %i\n", rt_entry->seqNum);
    printf("\t nextHopAddress: %s\n", netaddr_to_string(&nbuf, &(rt_entry->nextHopAddress)));
    printf("\t lastUsed: %i\n", rt_entry->lastUsed);
    printf("\t expirationTime: %i\n", rt_entry->expirationTime);
    printf("\t broken: %d\n", rt_entry->broken);
    printf("\t metricType: %i\n", rt_entry->metricType);
    printf("\t metric: %d\n", rt_entry->metric);
    printf("\t state: %d\n", rt_entry->state);
}