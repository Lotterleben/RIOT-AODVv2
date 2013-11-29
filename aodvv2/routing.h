/*
 * Cobbled-together routing table.
 * This is neither efficient nor elegant, but until RIOT gets their own native
 * RT, this will have to do.
 */

/* Sonix: Wie wärs mit Linked List von Key-Value-Elementen? */

/*
 * A route table entry (i.e., a route) may be in one of the following
   states:
 */
enum routing_table_states {
    ROUTE_STATE_ACTIVE,
    ROUTE_STATE_IDLE,
    ROUTE_STATE_EXPIRED,
    ROUTE_STATE_BROKEN,
    ROUTE_STATE_TIMED
}

/* contains all fields of a routing table entry */
/*TODO: 
- eliminate unnecessary fields (telweise schon erledigt)
- determine correct type for everything that's void*/
typedef struct {
    void address; 
    uint8_t prefixLength; //should be long enough, no?
    uint8_t seqNum;
    void nextHopAddress;
    void lastUsed; // use timer thingy for this?
    void expirationTime; // use timer thingy for this?
    bool broken;
    uint8_t metricType;
    uint8_t metric;
    uint8_t state; /* see routing_table_states */
} routing_t;

int init_routingtable();
int clear_routingtable();
get_routing_entry(void addr);
int update_routing_entry(); // wie mach ich das am elegantesten?
int delete_routing_entry(void addr);
