#include "router.h"

void Router(/* arguments */) {
    FILE *fd;
    fd = fopen("stage1.router1.out", "w+");

    /* bind a socket and get a dynamic UDP port*/
    int sock = socket(AF_INET, SOCK_DGRAM, 0);

    struct sockaddr_in addr;
    socklen_t addrlen = sizeof(addr);

    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = inet_addr("127.0.0.1");
    addr.sin_port = htons(0);
    bind(sock, (struct sockaddr*)&addr, sizeof(addr));
    getsockname(sock, (struct sockaddr*)&addr, &addrlen);

    short unsigned int router_port;
    router_port = addr.sin_port;

    LOG(fd, "router: %d, pid: %d, port: %d\n", num_routers, getpid(), router_port);

    /* send port to Proxy*/
    struct sockaddr_in proxyAddr;
    memset(&proxyAddr, 0, sizeof(proxyAddr));
    proxyAddr.sin_family = AF_INET;
    proxyAddr.sin_addr.s_addr = inet_addr("127.0.0.1");
    proxyAddr.sin_port = udp_port;

    //TODO: string !!!!!
    char buffer[BUF_SIZE];
    sprintf(buffer, "%d", getpid());
    std::cout << "Send: " << buffer << "\n";
    std::cout << "Length: " << strlen(buffer) << "\n";
    sendto(sock, buffer, strlen(buffer), 0, (struct sockaddr*) &proxyAddr, addrlen);

    //closesocket(sock);
    fclose(fd);
    return;
}
