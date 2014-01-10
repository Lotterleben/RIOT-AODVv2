#include "sender.h"

#define ENABLE_DEBUG (1)
#include "debug.h"

static struct autobuf _hexbuf;
static int sock;
static sockaddr6_t sa_sender, sa_bcast;
static struct netaddr origNode, targNode;
static uint16_t seqNum; 

/* helper functions */
void init_writer(void);
void send_udp(void *buffer, size_t length);
static void write_packet(struct rfc5444_writer *wr __attribute__ ((unused)),
        struct rfc5444_writer_target *iface __attribute__((unused)),
        void *buffer, size_t length);
static void print_ipv6_addr(const ipv6_addr_t *ipv6_addr);
static uint16_t get_node_id(void);


void sender_init(void)
{   
    /* experimental */

    destiny_init_transport_layer();
    sixlowpan_lowpan_init(TRANSCEIVER_NATIVE, get_node_id(), 0);

    init_seqNum();

    /* initialize buffer for hexdump */
    abuf_init(&_hexbuf);

    /* init reader and writer */
    reader_init();
    writer_init(write_packet);

    /* init my own address */
    sa_sender.sin6_family = AF_INET6;
    sa_sender.sin6_port = HTONS(MANET_PORT);

    ipv6_addr_t local_addr = {0};
    ipv6_iface_get_best_src_addr(&local_addr, &sa_sender.sin6_addr);

    sa_bcast.sin6_family = AF_INET6;
    ipv6_addr_set_all_nodes_addr(&sa_bcast.sin6_addr); // TODO: does this make sense?


    DEBUG("[aodvv2] My IP address is:\n");
    print_ipv6_addr(&sa_bcast.sin6_addr);

    /* init socket */
    sock = destiny_socket(PF_INET6, SOCK_DGRAM, IPPROTO_UDP);

    if(-1 == sock) {
        DEBUG("Error Creating Socket!");
        exit(EXIT_FAILURE);
    }
}

/* 
  TODO
  The RREQ (with updated fields as specified above>) SHOULD be sent
  to the IP multicast address LL-MANET-Routers [RFC5498]
*/

void send_rreq(struct netaddr* origNode, struct netaddr* targNode)
{
    DEBUG("[aodvv2] sending RREQ...\n");

    writer_send_rreq(origNode, targNode);

    /* cleanup */
    reader_cleanup();
    writer_cleanup();
    abuf_free(&_hexbuf);

    DEBUG("[aodvv2] RREQ sent\n");
}

void send_rrep(struct aodvv2_packet_data* packet_data, struct netaddr* next_hop)
{
    DEBUG("[aodvv2] sending RREP...\n");
    // TODO: use next_hop

    writer_send_rrep(packet_data);

    /* cleanup */
    //reader_cleanup();
    //writer_cleanup();
    //abuf_free(&_hexbuf);

    DEBUG("[aodvv2] RREP sent\n");
}

void receive_udp(char *str)
{
    sockaddr6_t sa = {0};
    sa.sin6_family = AF_INET6;
    sa.sin6_port = HTONS(MANET_PORT);

    uint32_t fromlen;
    int32_t recvsize; 
    char buffer[256];

    DEBUG("initializing UDP server...\n");
    sock = destiny_socket(PF_INET6, SOCK_DGRAM, IPPROTO_UDP);
    fromlen = sizeof(sa);

    if(destiny_socket_bind(sock, &sa, sizeof(sa)) < 0 ) {
        DEBUG("Error: bind failed! Exiting.\n");
        destiny_socket_close(sock);
        exit(EXIT_FAILURE);
    }

    DEBUG("ready to receive packets.\n");

    for(;;){
        recvsize = destiny_socket_recvfrom(sock, &buffer, sizeof(buffer), 0, 
                                            &sa_bcast, &fromlen);
        //recvsize = destiny_socket_recv(sock, (void *)buffer, sizeof(buffer), 0);

        if(recvsize < 0) {
            DEBUG("Error receiving data!\n");
        }

        DEBUG("recvsize: %"PRIi32"\n ", recvsize);
        DEBUG("datagram: %s\n", buffer);
    }
}

/**
 * Handle the output of the RFC5444 packet creation process
 * @param wr
 * @param iface
 * @param buffer
 * @param length
 */
static void
write_packet(struct rfc5444_writer *wr __attribute__ ((unused)),
        struct rfc5444_writer_target *iface __attribute__((unused)),
        void *buffer, size_t length)
{
    DEBUG("[aodvv2] %s()\n", __func__);

    /* generate hexdump and human readable representation of packet
        and print to console */
    abuf_hexdump(&_hexbuf, "\t", buffer, length);
    rfc5444_print_direct(&_hexbuf, buffer, length);
    DEBUG("%s", abuf_getptr(&_hexbuf));

    //send_udp(buffer, length);

    /* parse packet */
    // TODO: dummy solution. insert proper address.
    struct netaddr addr;
    netaddr_from_string(&addr, "::111");
    reader_handle_packet(buffer, length, &addr);
}

void send_udp(void *buffer, size_t length)
{
    int bytes_sent;

    DEBUG("sending data...\n");
    bytes_sent = destiny_socket_sendto(sock, buffer, length, 0, &sa_bcast, 
                                       sizeof sa_bcast);

    if(bytes_sent < 0) {
        DEBUG("Error sending packet: % i\n", bytes_sent);
    }else{
        DEBUG("success!\n");
    }

    // übergangslösung
    destiny_socket_close(sock);
}

void init_seqNum(void)
{
    seqNum = 1;
}

/* NOTE: lock on m_seqnum before using this function! */
void inc_seqNum(void)
{   
    if (seqNum == 65535)
        seqNum = 1;
    else if (seqNum == 0)
        DEBUG("ERROR: SeqNum shouldn't be 0! \n"); // TODO handle properly
    else
        seqNum++;
}

/* NOTE: lock on m_seqnum before using this function! */
uint16_t get_seqNum(void)
{
    return seqNum;
}

/* 
 * returns -1 when s1 is smaller, 0 if equal, 1 if s1 is bigger.
 * reasoning:
    Suppose we have S1 and S2.
    **Naive Approach:**
    S1 < S2 : S2 is newer
    S1 > S2 : S1 is newer
    **but:**
    What if S1 = 65535 and S2 = 1? With our naive approach, S1 would be assumed 
    to be newer, which is clearly wrong.
    So we'll need some kind of "wiggle room" around the tipping point of 65535 and 1.
 */
int cmp_seqnum(uint32_t s1, uint32_t s2)
{
    int wiggle_room = 10;

    if (s1 < s2){
        if ((s1 <= wiggle_room) && (s2 >= 65535 - wiggle_room))
            return 1;
        return -1;
    }
    if (s1 > s2){
        if (s1 >= 65535 - wiggle_room && s2 <= wiggle_room)
            return -1;
        return 1;
    }
    return 0;
}

/*********** HELPERS **********************************************************/

static void print_ipv6_addr(const ipv6_addr_t *ipv6_addr)
{
    char addr_str[IPV6_MAX_ADDR_STR_LEN];
    printf("%s\n", ipv6_addr_to_str(addr_str, ipv6_addr));
}

static uint16_t get_node_id(void) {
    DEBUG("node id is: %d \n", getpid());
    return getpid();
}

