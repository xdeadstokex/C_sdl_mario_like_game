///////////////////////// NET_H /////////////////////////
// network_t     - holds socket handle
// net_init      - bind udp socket to port, set block mode
// net_close     - close socket and cleanup
// net_send      - send buf to ip:port
// net_recv      - receive buf, optionally get sender ip
// net_broadcast - send to all on LAN (255.255.255.255)
/////////////////////////////////////////////////////////

#ifndef NET_H
#define NET_H

#include <stdlib.h>
#include <string.h>

typedef struct {
    void* data;
} network_t;

#ifdef _WIN32

#include <winsock2.h>

static inline int net_init(network_t* net, unsigned short port, int blocking_mode) {
    WSADATA wsa;
    WSAStartup(MAKEWORD(2,2), &wsa);

    int* sock = malloc(sizeof(int));
    *sock = socket(AF_INET, SOCK_DGRAM, 0);

    u_long mode = blocking_mode ? 0 : 1;
    ioctlsocket(*sock, FIONBIO, &mode);

    int broadcast = 1;
    setsockopt(*sock, SOL_SOCKET, SO_BROADCAST, (void*)&broadcast, sizeof(broadcast));

    struct sockaddr_in addr = {0};
    addr.sin_family      = AF_INET;
    addr.sin_port        = htons(port);
    addr.sin_addr.s_addr = INADDR_ANY;
    bind(*sock, (struct sockaddr*)&addr, sizeof(addr));

    net->data = sock;
    return 0;
}

static inline void net_close(network_t* net) {
    closesocket(*(int*)net->data);
    WSACleanup();
    free(net->data);
}

static inline int net_send(network_t* net, const char* ip, unsigned short port, void* buf, int len) {
    struct sockaddr_in addr = {0};
    addr.sin_family      = AF_INET;
    addr.sin_port        = htons(port);
    addr.sin_addr.s_addr = inet_addr(ip);
    return sendto(*(int*)net->data, buf, len, 0, (struct sockaddr*)&addr, sizeof(addr));
}

static inline int net_recv(network_t* net, void* buf, int len, char* from_ip) {
    struct sockaddr_in addr = {0};
    int addr_len = sizeof(addr);
    int n = recvfrom(*(int*)net->data, buf, len, 0, (struct sockaddr*)&addr, &addr_len);
    if (from_ip && n > 0) strcpy(from_ip, inet_ntoa(addr.sin_addr));
    return n;
}

#else

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>

static inline int net_init(network_t* net, unsigned short port, int blocking_mode) {
    int* sock = malloc(sizeof(int));
    *sock = socket(AF_INET, SOCK_DGRAM, 0);

    if (!blocking_mode)
        fcntl(*sock, F_SETFL, O_NONBLOCK);

    int broadcast = 1;
    setsockopt(*sock, SOL_SOCKET, SO_BROADCAST, (void*)&broadcast, sizeof(broadcast));

    struct sockaddr_in addr = {0};
    addr.sin_family      = AF_INET;
    addr.sin_port        = htons(port);
    addr.sin_addr.s_addr = INADDR_ANY;
    bind(*sock, (struct sockaddr*)&addr, sizeof(addr));

    net->data = sock;
    return 0;
}

static inline void net_close(network_t* net) {
    close(*(int*)net->data);
    free(net->data);
}

static inline int net_send(network_t* net, const char* ip, unsigned short port, void* buf, int len) {
    struct sockaddr_in addr = {0};
    addr.sin_family      = AF_INET;
    addr.sin_port        = htons(port);
    addr.sin_addr.s_addr = inet_addr(ip);
    return sendto(*(int*)net->data, buf, len, 0, (struct sockaddr*)&addr, sizeof(addr));
}

static inline int net_recv(network_t* net, void* buf, int len, char* from_ip) {
    struct sockaddr_in addr = {0};
    socklen_t addr_len = sizeof(addr);
    int n = recvfrom(*(int*)net->data, buf, len, 0, (struct sockaddr*)&addr, &addr_len);
    if (from_ip && n > 0) strcpy(from_ip, inet_ntoa(addr.sin_addr));
    return n;
}

#endif

static inline int net_broadcast(network_t* net, unsigned short port, void* buf, int len) {
    return net_send(net, "255.255.255.255", port, buf, len);
}

#endif // NET_H