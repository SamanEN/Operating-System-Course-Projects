#include "udp.h"

#include <arpa/inet.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#include "consts.h"
#include "utils.h"

void broadcast_msg(const char* port, const char* msg) {
    struct sockaddr_in bc_address;
    int broadcast = 1, reuse_port = 1;

    int sock_fd = socket(AF_INET, SOCK_DGRAM, 0);
    setsockopt(sock_fd, SOL_SOCKET, SO_BROADCAST, &broadcast, sizeof(broadcast));
    setsockopt(sock_fd, SOL_SOCKET, SO_REUSEPORT, &reuse_port, sizeof(reuse_port));
    bc_address.sin_family = AF_INET;
    bc_address.sin_port = htons(8080);
    bc_address.sin_addr.s_addr = inet_addr("192.168.160.255");

    bind(sock_fd, (struct sockaddr*)&bc_address, sizeof(bc_address));

    if (sendto(sock_fd, msg, strlen(msg), 0, (struct sockaddr*)&bc_address, sizeof(bc_address)) == -1) {
        log_err(STDERR_FILENO, BROADCAST_FAILED, 1, 1);
        exit(EXIT_FAILURE);
    }
    close(sock_fd);
}

int connect_udp(const int port) {
    struct sockaddr_in bc_address;
    int broadcast = 1, reuse_port = 1;

    int sock_fd = socket(AF_INET, SOCK_DGRAM, 0);
    setsockopt(sock_fd, SOL_SOCKET, SO_BROADCAST, &broadcast, sizeof(broadcast));
    setsockopt(sock_fd, SOL_SOCKET, SO_REUSEPORT, &reuse_port, sizeof(reuse_port));
    bc_address.sin_family = AF_INET;
    bc_address.sin_port = htons(port);
    bc_address.sin_addr.s_addr = inet_addr("192.168.160.255");

    bind(sock_fd, (struct sockaddr*)&bc_address, sizeof(bc_address));

    return sock_fd;
}

char* rcv_udp(int sock_fd) {
    char buf[1024] = {0};

    char* result = NULL;
    int total_bytes_rcvd = 0;

    for (;;) {
        int recv_bytes = recv(sock_fd, buf, 1024, 0);
        if (recv_bytes <= 0)
            return result;
        result = (char*)realloc(result, recv_bytes);
        memcpy(result, buf, recv_bytes);
        total_bytes_rcvd += recv_bytes;
        if (recv_bytes == 1024) {
            fd_set read_fd_set;
            FD_ZERO(&read_fd_set);
            FD_SET(sock_fd, &read_fd_set);
            struct timeval t = {0, 0};
            select(sock_fd, &read_fd_set, NULL, NULL, &t);
            if (FD_ISSET(sock_fd, &read_fd_set))
                continue;
        }
        break;
    }
    result = (char*)realloc(result, total_bytes_rcvd + 1);
    result[total_bytes_rcvd] = '\0';

    return result;
}
