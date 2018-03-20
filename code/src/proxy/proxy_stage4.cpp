#include "proxy.h"

/**************************************************************************
* proxy stage 4:
* Recieve the setup packet from multiple router
* should wait until get info from all the routers
**************************************************************************/
int ProxyClass::stage4()
{
        char buffer[BUF_SIZE];
        int count = num_routers;
        routers_pid =  (int*)malloc(sizeof(int)*num_routers);
        routers_port = (int*)malloc(sizeof(int)*num_routers);

        while (count)
        {
                int strLen = recvfrom(sock, buffer, BUF_SIZE, 0,
                                      (struct sockaddr *)routerAddr, (socklen_t *)&nSize);

                string data(buffer);
                int router_pid = atoi(data.substr(1,data.length()-1).c_str());
                int router_id =  atoi(data.substr(0,1).c_str());
                routers_pid[router_id] = router_pid;
                routers_port[router_id] = routerAddr->sin_port;

                printf("\nRead a packet from Router%d, packet length:%d\n", router_id+1, strLen);
                printf("    router ip is %s\n", inet_ntoa(eth[router_id].sin_addr));

                if (stage > 4)
                        LOG(logfd, "router: %d, pid: %d, port: %d, IP:%s\n",
                            router_id+1, router_pid, routerAddr->sin_port, inet_ntoa(eth[router_id].sin_addr));
                else
                        LOG(logfd, "router: %d, pid: %d, port: %d\n",
                            router_id+1, router_pid, routerAddr->sin_port);

                count--;
        }

        for (size_t i = 0; i < num_routers; i++) {
                printf("router port: %d\n", routers_port[i]);
        }
        return 1;
};

/**************************************************************************
* When proxy read a packet from tunnel relay it to router
* Should check the destination IP after stage 4
*
* For destinations that are routers, send them directly,
* otherwise do as follows:
* We require that you hash the destination IP address across the routers
* that are provided. To do so, treat the destination IP address as a
* unsigned 32-bit number and take it MOD the number of routers, then add one.
* (So with 4 routers, IP address 1.0.0.5 will map to 2, because 16,777,221 MOD 4 is 1.)
**************************************************************************/
struct sockaddr_in* ProxyClass::hashDstIP(string ip)
{
        struct sockaddr_in* ret = (struct sockaddr_in*)malloc(sizeof(struct sockaddr_in));
        ret->sin_family = AF_INET;
        ret->sin_addr.s_addr = routerAddr->sin_addr.s_addr;

        printf("IP: %s\n", ip.c_str());

        //For destinations that are routers, send them directly
        if (ip.length()>12)
        {
                int num = 0;
                try {
                        num = stoi(ip.substr(10,1));
                } catch (invalid_argument& e) {
                        //std::cout << "conversion failed" << std::endl;
                }

                if (ip.substr(0,10) == "192.168.20" && (num > 0 && num < num_routers))
                {
                        ret->sin_port = routers_port[num-1];
                        return ret;
                }
        }

        //otherwise do as follows:
        struct sockaddr_in tmp;
        memset(&tmp, 0, sizeof(struct sockaddr_in));
        //inet_aton(ip.c_str(), &tmp.sin_addr);
        //tmp.sin_addr.s_addr = inet_addr(ip.c_str());
        inet_pton(AF_INET, ip.c_str(), &tmp.sin_addr);

        //unsigned 32-bit number
        long long IntIp = ((tmp.sin_addr.s_addr >> 0)  & 0xFF) * 256*256*256 +
                          ((tmp.sin_addr.s_addr >> 8)  & 0xFF) * 256*256 +
                          ((tmp.sin_addr.s_addr >> 16) & 0xFF) * 256 +
                          ((tmp.sin_addr.s_addr >> 24) & 0xFF);

        int target_id = IntIp % num_routers;
        ret->sin_port = routers_port[target_id];
        return ret;
}
