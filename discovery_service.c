#include "structs.h"
#include <pthread.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include "management_service.h"

#define PORT 4000
#define RESPONSE_PORT 4001
#define DISCOVERY_TYPE 1
#define CONFIRMED_TYPE 2

// Function to send a discovery message
void send_discovery_msg(int sockfd, struct sockaddr_in *addr, socklen_t len)
{
    packet msg;
    msg.type = DISCOVERY_TYPE;
    msg.seqn = 0;
    msg.length = 0;
    msg.timestamp = time(NULL);
    msg._payload = NULL;

    int broadcast_enable = 1;
    setsockopt(sockfd, SOL_SOCKET, SO_BROADCAST, &broadcast_enable, sizeof(broadcast_enable));
    addr->sin_addr.s_addr = htonl(INADDR_BROADCAST);
    int n = sendto(sockfd, &msg, sizeof(packet), 0, (struct sockaddr *)addr, len);
    if (n < 0)
    {
        printf("ERROR on sendto");
    }
    else
    {
        printf("Successfully sent broadcast message\n");
    }
}



void *listen_discovery(void *args)
{
    int sockfd;
    struct sockaddr_in serv_addr;
    socklen_t manlen = sizeof(serv_addr);
    packet msg;

    // Create socket
    if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) == -1)
    {
        printf("Error opening socket");
        pthread_exit(NULL);
    }

    // Bind socket to address and port
    memset((char *)&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    serv_addr.sin_port = htons(PORT);

    if (bind(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
    {
        printf("Error on binding");
        pthread_exit(NULL);
    }

    int message_shown = 0;
    while (1)
    {
        if (!message_shown)
        {
            printf("Aguardando um novo participante entrar...\n");
            message_shown = 1;
        }
        struct sockaddr_in cli_addr;
        socklen_t clilen = sizeof(cli_addr);

        // Receive message
        int n = recvfrom(sockfd, &msg, sizeof(msg), 0, (struct sockaddr *)&cli_addr, &clilen);
        if (n < 0)
        {
            printf("Error on recvfrom");
            pthread_exit(NULL);
        }
        else
        {
            printf("Successfully sent broadcast message\n");
        }

        
        if (msg.type == DISCOVERY_TYPE)
        {
            char hostname[256], ip_address[16], mac_address[18];
            getnameinfo((struct sockaddr *)&cli_addr, clilen, hostname, sizeof(hostname), NULL, 0, 0);
            inet_ntop(AF_INET, &(cli_addr.sin_addr.s_addr), ip_address, INET_ADDRSTRLEN);
            add_participant(hostname, ip_address, mac_address, 1);

        }
    }
    close(sockfd);
    pthread_exit(NULL);
}


void participant_start()
{
    // Listen for manager broadcast
    int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(PORT);
    socklen_t manager_addrlen = sizeof(addr);
    send_discovery_msg(sockfd, &addr, manager_addrlen);
    close(sockfd);
}

void get_mac_address(char *mac_address)
{
    struct ifreq ifr;
    struct ifconf ifc;
    char buf[1024];
    int success = 0;

    int sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_IP);
    if (sock == -1)
    { 
    };

    ifc.ifc_len = sizeof(buf);
    ifc.ifc_buf = buf;
    if (ioctl(sock, SIOCGIFCONF, &ifc) == -1)
    { 
    }

    struct ifreq *it = ifc.ifc_req;
    const struct ifreq *const end = it + (ifc.ifc_len / sizeof(struct ifreq));

    for (; it != end; ++it)
    {
        strcpy(ifr.ifr_name, it->ifr_name);
        if (ioctl(sock, SIOCGIFFLAGS, &ifr) == 0)
        {
            if (!(ifr.ifr_flags & IFF_LOOPBACK))
            { 
                if (ioctl(sock, SIOCGIFHWADDR, &ifr) == 0)
                {
                    success = 1;
                    break;
                }
            }
        }
        else
        { 
        }
    }

    if (success)
    {
        memcpy(mac_address, ifr.ifr_hwaddr.sa_data, 6);
        sprintf(mac_address, "%02x:%02x:%02x:%02x:%02x:%02x",
                (unsigned char)mac_address[0],
                (unsigned char)mac_address[1],
                (unsigned char)mac_address[2],
                (unsigned char)mac_address[3],
                (unsigned char)mac_address[4],
                (unsigned char)mac_address[5]);
    }
}