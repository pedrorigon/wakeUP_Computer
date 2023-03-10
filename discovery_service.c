#include "structs.h"
#include <pthread.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include "management_service.h"
#include "monitoring_service.h"
#include "discovery_service.h"
#include "user_interface.h"

participant manager = {0};


void send_type_msg(char mac_address[18], char ip_address[16], int port, int msg_type)
{
    packet msg;
    int sockfd;
    struct hostent *server;
    msg.type = msg_type;
    msg.seqn = 0;
    msg.length = 0;
    msg.timestamp = time(NULL);
    msg._payload = NULL;
    strcpy(msg.mac_address, mac_address);
    msg.status = 1;

    server = gethostbyname(ip_address);
    if (!server)
    {
        printf("ERROR: Failed to resolve hostname: %s", ip_address);
    }
    if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) == -1)
    {
        printf("ERROR: Failed to create socket");
    }
    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr = *((struct in_addr *)server->h_addr);
    bzero(&(addr.sin_zero), 8);

    int n = sendto(sockfd, &msg, sizeof(packet), 0, (struct sockaddr *)&addr, sizeof(addr));
    if (n < 0)
    {
        printf("ERROR on sendto");
    }
}

void send_goodbye_msg(void) {
    if(!manager.status) {
        puts("Sem manager!");
        return;
    }
    
    char mac_adress[18];
    get_mac_address(mac_adress);

    send_type_msg(mac_adress, manager.ip_address, RESPONSE_PORT_MONITORING, PROGRAM_EXIT_TYPE);
}

// Function to send a discovery message

void send_discovery_msg(int sockfd, struct sockaddr_in *addr, socklen_t len, char mac_address[18])
{
    packet msg;
    msg.type = DISCOVERY_TYPE;
    msg.seqn = 0;
    msg.length = 0;
    msg.timestamp = time(NULL);
    msg._payload = NULL;
    strcpy(msg.mac_address, mac_address);
    msg.status = 1;

    int broadcast_enable = 1;
    setsockopt(sockfd, SOL_SOCKET, SO_BROADCAST, &broadcast_enable, sizeof(broadcast_enable));
    addr->sin_addr.s_addr = htonl(INADDR_BROADCAST);

    while (1)
    {
        usleep(1000000);
        int n = sendto(sockfd, &msg, sizeof(packet), 0, (struct sockaddr *)addr, len);
        if (n < 0)
        {
            printf("ERROR on sendto");
        }
    }
    close(sockfd);
}

void *listen_discovery(void *args)
{
    int sockfd;
    struct sockaddr_in serv_addr;
    socklen_t manlen = sizeof(serv_addr);
    packet msg;
    participant new_participant;

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
            printf("\n");
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

        if (msg.type == DISCOVERY_TYPE)
        {
            char hostname[256], ip_address[16];
            getnameinfo((struct sockaddr *)&cli_addr, clilen, hostname, sizeof(hostname), NULL, 0, 0);
            inet_ntop(AF_INET, &(cli_addr.sin_addr.s_addr), ip_address, INET_ADDRSTRLEN);
            int index = find_participant(msg.mac_address);
            if (index == -1) //n??o achou na lista 
            {
                add_participant(hostname, ip_address, msg.mac_address, msg.status, PARTICIPANT_TIMEOUT);
            }
                char mac_adress_manager[18];
                get_mac_address(mac_adress_manager);

                send_type_msg(mac_adress_manager, ip_address, RESPONSE_PORT, CONFIRMED_TYPE);
            }
        }
        close(sockfd);
        pthread_exit(NULL);
    }

    void participant_start()
    {
        // Listen for manager broadcast
        struct hostent *host_entry;
        int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
        struct sockaddr_in addr;
        addr.sin_family = AF_INET;
        addr.sin_addr.s_addr = INADDR_ANY;
        addr.sin_port = htons(PORT);
        socklen_t manager_addrlen = sizeof(addr);

        char hostname[256], ip_address[16], mac_address[18];

        // Obt??m o hostname
        // gethostname(hostname, 256);

        getnameinfo((struct sockaddr *)&addr, manager_addrlen, hostname, sizeof(hostname), NULL, 0, 0);
        inet_ntop(AF_INET, &(addr.sin_addr.s_addr), ip_address, INET_ADDRSTRLEN);

        // Obt??m informa????es sobre o host
        // host_entry = gethostbyname(hostname);

        // Obt??m o endere??o IP
        // inet_ntop(AF_INET, &(addr.sin_addr.s_addr), ip_address, INET_ADDRSTRLEN);

        // getnameinfo((struct sockaddr *)&addr, manager_addrlen, hostname, sizeof(hostname), NULL, 0, 0);
        // inet_ntop(AF_INET, &(addr.sin_addr.s_addr), ip_address, INET_ADDRSTRLEN);
        get_mac_address(mac_address);
        // printf("o ip aqui eh\n: %s", ip_address);
        add_participant_noprint(hostname, ip_address, mac_address, 1, PARTICIPANT_TIMEOUT);
        send_discovery_msg(sockfd, &addr, manager_addrlen, mac_address);

        // send_discovery_msg(sockfd, &addr, manager_addrlen);
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
            perror("socket");
            exit(EXIT_FAILURE);
        }

        ifc.ifc_len = sizeof(buf);
        ifc.ifc_buf = buf;
        if (ioctl(sock, SIOCGIFCONF, &ifc) == -1)
        {
            perror("ioctl");
            exit(EXIT_FAILURE);
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
                perror("ioctl");
                exit(EXIT_FAILURE);
            }
        }
        if (success)
        {
            char formatted_mac_address[18];
            memcpy(formatted_mac_address, ifr.ifr_hwaddr.sa_data, 6);
            sprintf(formatted_mac_address, "%02x:%02x:%02x:%02x:%02x:%02x",
                    (unsigned char)formatted_mac_address[0],
                    (unsigned char)formatted_mac_address[1],
                    (unsigned char)formatted_mac_address[2],
                    (unsigned char)formatted_mac_address[3],
                    (unsigned char)formatted_mac_address[4],
                    (unsigned char)formatted_mac_address[5]);
            strcpy(mac_address, formatted_mac_address);
        }
    }

    void *listen_Confirmed(void *args)
    {
        int sockfd;
        struct sockaddr_in serv_addr;
        socklen_t manlen = sizeof(serv_addr);
        packet msg;
        participant new_participant1;

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
        serv_addr.sin_port = htons(RESPONSE_PORT);
        bzero(&(serv_addr.sin_zero), 8);

        if (bind(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
        {
            printf("Error on binding");
            pthread_exit(NULL);
        }
        struct sockaddr_in cli_addr;
        socklen_t clilen = sizeof(cli_addr);
        char mac_address[18];
        while (1)
        {

            // Receive message
            int n = recvfrom(sockfd, &msg, sizeof(msg), 0, (struct sockaddr *)&cli_addr, &clilen);
            if (n < 0)
            {
                printf("Error on recvfrom");
                pthread_exit(NULL);
            }
            if (msg.type == CONFIRMED_TYPE)
            {
                char hostname[256], ip_address[16];
                getnameinfo((struct sockaddr *)&cli_addr, clilen, hostname, sizeof(hostname), NULL, 0, 0);
                inet_ntop(AF_INET, &(cli_addr.sin_addr.s_addr), ip_address, INET_ADDRSTRLEN);
                /*int att_dados = add_participant(hostname, ip_address, msg.mac_address, msg.status);
                if (att_dados == 0)
                {
                    // printf("----------------------------------\n");
                    printf("Esse ?? o endere??o de seu Manager!\n");
                    printf("----------------------------------\n");
                }*/
                // char mac_address[18];
                if (strcmp(mac_address, msg.mac_address) != 0)
                {
                    /*printf("------------------------------------------------\n");
                    printf("       Esse ?? o endere??o de seu Manager!\n");
                    printf("------------------------------------------------\n");
                    printf("Hostname: %s\n", hostname);
                    printf("IP address: %s\n", ip_address);
                    printf("MAC address: %s\n", msg.mac_address);
                    strcpy(mac_address, msg.mac_address);
                    if (msg.status == 1)
                    {
                        printf("Status: awaken\n");
                    }
                    else
                    {
                        printf("Status: asleep\n");
                    }
                    printf("------------------------------------------------\n");
                    printf("\n");*/
                    strcpy(manager.hostname, hostname);
                    strcpy(manager.ip_address, ip_address);
                    strcpy(manager.mac_address, msg.mac_address);
                    manager.status = 1;
                    sem_post(&sem_update_interface);
                }
            }
        }
        close(sockfd);
        pthread_exit(NULL);
    }