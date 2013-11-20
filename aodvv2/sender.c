#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include <inttypes.h>

#include "destiny/socket.h"


void udp_receive(char *str){

}

void udp_send(char *str){

    int sock;
    sockaddr6_t sockaddr;
    ipv6_addr_t ipaddr;

    int address, bytes_sent;
    char text[] = "abcdefghijklmnopqrstuvwxyz0123456789!-=$%&/()";

    sock = destiny_socket(AF_INET6, SOCK_DGRAM, IPPROTO_UDP );

    if (!sock){
        perror("Error opening socket");
        exit(EXIT_FAILURE);
    }

    /* set own ip address */
    ipv6_addr_init(&ipaddr, 0xabcd, 0x0, 0x0, 0x0, 0x3612, 0x00ff, 0xfe00, (uint16_t) address);
    print_ipv6_addr(&ipaddr);

    sockaddr.sin6_family = AF_INET6;
    memcpy(&sockaddr.sin6_addr, &ipaddr, 16); /* why the memcpy? */
    sockaddr.sin6_port = HTONS(0xf4);

    printf("sending data...\n");
    bytes_sent = destiny_socket_sendto(sock, (char *)text, 
                                        strlen((char *)text) + 1, 0, &sockaddr, 
                                        sizeof sockaddr);
    if(bytes_sent < 0) {
        perror("Error sending packet");
    }

    destiny_socket_close(sock);
}
