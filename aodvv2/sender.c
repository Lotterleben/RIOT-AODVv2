#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "ltc4150.h"

#include "destiny/socket.h" // need
#include "net_help.h" // need

#include "sender.h"

void send_udp(char *str){

    timex_t start, end, total;
    long secs;
    int sock;
    sockaddr6_t sa;
    ipv6_addr_t ipaddr;
    int bytes_sent;
    int address, count;
    char text[] = "abcdefghijklmnopqrstuvwxyz0123456789!-=$%&/()";
    sscanf(str, "send_udp %i %i %s", &count, &address, text);

    sock = destiny_socket(PF_INET6, SOCK_DGRAM, IPPROTO_UDP);

    if(-1 == sock) {
        printf("Error Creating Socket!");
        exit(EXIT_FAILURE);
    }

    memset(&sa, 0, sizeof sa);

    ipv6_addr_init(&ipaddr, 0xabcd, 0x0, 0x0, 0x0, 0x3612, 0x00ff, 0xfe00, (uint16_t)address);

    sa.sin6_family = AF_INET;
    memcpy(&sa.sin6_addr, &ipaddr, 16);
    sa.sin6_port = HTONS(0xf4);
    ltc4150_start();
    printf("Start power: %f\n", ltc4150_get_total_Joule());
    vtimer_now(&start);

    for(int i = 0; i < count; i++) {
        bytes_sent = destiny_socket_sendto(sock, (char *)text, 
                                           strlen((char *)text) + 1, 0, &sa, 
                                           sizeof sa);

        if(bytes_sent < 0) {
            printf("Error sending packet!\n");
        }

        /*  hwtimer_wait(20*1000); */
    }

    vtimer_now(&end);
    total = timex_sub(end, start);
    secs = total.microseconds / 1000000;
    printf("Used power: %f\n", ltc4150_get_total_Joule());
    printf("Start: %lu, End: %lu, Total: %lu\n", start.microseconds, end.microseconds, total.microseconds);
    secs = total.microseconds / 1000000;
    if (!secs) {
        puts("Transmission in no time!");
    }
    else {
        printf("Time: %lu seconds, Bandwidth: %lu byte/second\n", secs, (count * 48) / secs);
    }
    destiny_socket_close(sock);
}
