#include "proxy.h"

void Proxy(int sock, struct sockaddr_in *addr) {
    FILE *fd;
    fd = fopen("stage1.proxy.out", "w+");

    LOG(fd, "proxy port: %d\n", udp_port);

    struct sockaddr_in recvAddr;
    int nSize = sizeof(struct sockaddr);
    char buffer[BUF_SIZE];
    char str[BUF_SIZE];
    int strLen = recvfrom(sock, buffer, BUF_SIZE, 0, (struct sockaddr*) &recvAddr, (socklen_t*) &nSize);
    strncpy(str, buffer, strLen);
    str[strLen] = '\0';

    int pid = atoi(buffer);

    LOG(fd, "router: %d, pid: %d, port: %d\n", num_routers, pid, recvAddr.sin_port);

    //closesocket(sock);
    fclose(fd);
    return;
}
