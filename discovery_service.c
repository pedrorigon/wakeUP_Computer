#include "structs.h"
#include <pthread.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include "management_service.h"
#include "monitoring_service.h"
#include "discovery_service.h"
#include "user_interface.h"
#include "nanopb/pb_common.h"
#include "nanopb/pb_encode.h"
#include "nanopb/pb_decode.h"
#include "messages.pb.h"

#define DISCOVERY_PORT 4000
#define RESPONSE_PORT 4001
#define DISCOVERY_TYPE 1
#define CONFIRMED_TYPE 2

#define BUFFER_SIZE (Packet_size)

participant manager = {0};

int send_message(Packet *packet, char ip_address[16], in_port_t port, bool broadcast)
{
    int sockfd;

    if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) == -1)
    {
        printf("ERROR: Failed to create socket");
    }

   
    if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) == -1)
    {
        printf("ERROR: Failed to create socket");
    }

    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);

    if(broadcast) {
        int broadcast_enable = 1;
        setsockopt(sockfd, SOL_SOCKET, SO_BROADCAST, &broadcast_enable, sizeof(broadcast_enable));
        addr.sin_addr.s_addr = htonl(INADDR_BROADCAST);
    } else {
        struct hostent *server;
        server = gethostbyname(ip_address);

        if (!server)
        {
            printf("ERROR: Failed to resolve hostname: %s", ip_address);
        }
        addr.sin_addr = *((struct in_addr *)server->h_addr);
    }
    bzero(&(addr.sin_zero), 8);

    uint8_t buffer[BUFFER_SIZE];
    pb_ostream_t stream = pb_ostream_from_buffer(buffer, Packet_size);
    bool status = pb_encode(&stream, Packet_fields, packet);
    int message_length = stream.bytes_written;

    if (!status) {
        printf("Encoding failed: %s\n", PB_GET_ERROR(&stream));
        return -1;
    }

    int rc = sendto(sockfd, buffer, message_length, 0, (struct sockaddr *)&addr, sizeof(addr));
    close(sockfd);
    return rc;
}

// Function to send a discovery message

void send_discovery_msg(char mac_address[18])
{
    uint32_t seqn = 0;
    while (1)
    {
        usleep(1000000);
        Packet packet = { 
            .seqn = seqn,
            .timestamp = time(NULL),
            .type = MessageType_DISCOVERY,
            .which_payload = Packet_discovery_tag
         };
        
        Discovery discovery = {
            .status = MachineStatus_WOKE
        };
        
        strcpy(discovery.mac_address, mac_address);
        packet.payload.discovery = discovery;

        int n = send_message(&packet, "255.255.255.255", DISCOVERY_PORT, true);

        if (n < 0)
        {
            printf("ERROR on send_message");
        }
        seqn++;
    }
}

void *listen_discovery(void *args)
{
    int sockfd;
    struct sockaddr_in serv_addr;
    socklen_t manlen = sizeof(serv_addr);
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
    serv_addr.sin_port = htons(DISCOVERY_PORT);

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

        uint8_t buffer[BUFFER_SIZE] = {0};
        // Receive message
        int n = recvfrom(sockfd, buffer, Packet_size, 0, (struct sockaddr *)&cli_addr, &clilen);
        if (n < 0)
        {
            printf("Error on recvfrom");
            pthread_exit(NULL);
        }

        pb_istream_t stream = pb_istream_from_buffer(buffer, n);
        Packet packet = Packet_init_zero;
        int status = pb_decode(&stream, Packet_fields, &packet);
        
        /* Check for errors... */
        if (!status)
        {
            printf("Decoding failed: %s\n", PB_GET_ERROR(&stream));
            return 1;
        }

        if (packet.type == MessageType_DISCOVERY)
        {
            char hostname[256], ip_address[16];
            getnameinfo((struct sockaddr *)&cli_addr, clilen, hostname, sizeof(hostname), NULL, 0, 0);
            inet_ntop(AF_INET, &(cli_addr.sin_addr.s_addr), ip_address, INET_ADDRSTRLEN);
            add_participant(hostname, ip_address, packet.payload.discovery.mac_address, packet.payload.discovery.status, PARTICIPANT_TIMEOUT);
            char mac_adress_manager[18];
            get_mac_address(mac_adress_manager);

            Packet confirmation_packet = {
                .timestamp = time(NULL),
                .type = MessageType_CONFIRMATION,
                .which_payload = Packet_confirmation_tag,
            };
            Confirmation confirmation = {
                .status = MachineStatus_WOKE
            };
            strcpy(confirmation.mac_address, mac_adress_manager);
            confirmation_packet.payload.confirmation = confirmation;            
            send_message(&confirmation_packet, "255.255.255.255", RESPONSE_PORT, true);
        }
    }
    close(sockfd);
    pthread_exit(NULL);
}

void participant_start()
{
    // Listen for manager broadcast
    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(DISCOVERY_PORT);
    socklen_t manager_addrlen = sizeof(struct sockaddr_in);

    char hostname[256], ip_address[16], mac_address[18];

    // Obtém o hostname
    // gethostname(hostname, 256);

    getnameinfo((struct sockaddr *)&addr, manager_addrlen, hostname, sizeof(hostname), NULL, 0, 0);
    inet_ntop(AF_INET, &(addr.sin_addr.s_addr), ip_address, INET_ADDRSTRLEN);

    // Obtém informações sobre o host
    // host_entry = gethostbyname(hostname);

    // Obtém o endereço IP
    // inet_ntop(AF_INET, &(addr.sin_addr.s_addr), ip_address, INET_ADDRSTRLEN);

    // getnameinfo((struct sockaddr *)&addr, manager_addrlen, hostname, sizeof(hostname), NULL, 0, 0);
    // inet_ntop(AF_INET, &(addr.sin_addr.s_addr), ip_address, INET_ADDRSTRLEN);
    get_mac_address(mac_address);
    // printf("o ip aqui eh\n: %s", ip_address);
    add_participant_noprint(hostname, ip_address, mac_address, 1, PARTICIPANT_TIMEOUT);

    Discovery discovery = {
        .status = MachineStatus_WOKE
    };

    strcpy(discovery.hostname, hostname);
    strcpy(discovery.mac_address, mac_address);

    Packet packet = {
        .timestamp = time(NULL),
        .type = MessageType_DISCOVERY,
        .which_payload = Packet_discovery_tag,
        .payload.discovery = discovery
    };

    packet.payload.discovery = discovery;
    send_message(&packet, "255.255.255.255", DISCOVERY_PORT, true);

    //send_discovery_msg(mac_address);
}

void *listen_Confirmed(void *args)
{
    int sockfd;
    struct sockaddr_in serv_addr;
    socklen_t manlen = sizeof(serv_addr);
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
        
        uint8_t buffer[BUFFER_SIZE] = {0};
        // Receive message
        int n = recvfrom(sockfd, buffer, sizeof(buffer), 0, (struct sockaddr *)&cli_addr, &clilen);
        if (n < 0)
        {
            printf("Error on recvfrom");
            pthread_exit(NULL);
        }

        pb_istream_t stream = pb_istream_from_buffer(buffer, n);
        Packet packet = Packet_init_zero;
        int status = pb_decode(&stream, Packet_fields, &packet);
        if (!status)
        {
            printf("Decoding failed: %s\n", PB_GET_ERROR(&stream));
            return 1;
        }

        printf("Packet type aaaa: %d\n", packet.type);
        if (packet.type == MessageType_CONFIRMATION)
        {
            char hostname[256], ip_address[16];
            getnameinfo((struct sockaddr *)&cli_addr, clilen, hostname, sizeof(hostname), NULL, 0, 0);
            inet_ntop(AF_INET, &(cli_addr.sin_addr.s_addr), ip_address, INET_ADDRSTRLEN);
            puts("Confirmation received!");
            //printf("mac_address: %s, payload address: %s\n", mac_address, packet.payload.confirmation.mac_address);
            printf("status: %s\n", packet.payload.discovery.mac_address);
            if (strcmp(mac_address, packet.payload.confirmation.mac_address) != 0)
            {
                strcpy(manager.hostname, hostname);
                strcpy(manager.ip_address, ip_address);
                strcpy(manager.mac_address, packet.payload.confirmation.mac_address);
                manager.status = 1;
                sem_post(&sem_update_interface);
            }
            
        }
    }
    close(sockfd);
    pthread_exit(NULL);
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
