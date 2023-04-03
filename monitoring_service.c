#include "structs.h"
#include <pthread.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include "management_service.h"
#include "discovery_service.h"
#include "monitoring_service.h"

pthread_t confirmed_thread;
pthread_t msg_discovery_thread;
pthread_t listen_monitoring_thread;
pthread_t user_interface_control;
pthread_t exit_participants_control;
pthread_t monitor_manager_status_thread;
pthread_t election_listener_thread;
pthread_t synchronization_client_thread;

void send_confirmed_status_msg(struct sockaddr_in *addr, socklen_t len, char mac_address[18], char ip_address[16])
{
    packet msg;
    int sockfd;
    struct hostent *server;
    msg.type = CONFIRMED_STATUS_TYPE;
    msg.seqn = 0;
    msg.length = 0;
    msg.timestamp = time(NULL);
    msg._payload = NULL;
    strcpy(msg.mac_address, mac_address);
    char ip_address_participant[16];
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
    addr->sin_family = AF_INET;
    addr->sin_port = htons(RESPONSE_PORT_MONITORING);
    addr->sin_addr = *((struct in_addr *)server->h_addr);
    bzero(&(addr->sin_zero), 8);

    int n = sendto(sockfd, &msg, sizeof(packet), 0, (struct sockaddr *)addr, len);
    if (n < 0)
    {
        printf("ERROR on sendto");
    }
}

// Function to send a discovery message
void send_monitoring_msg(int sockfd, struct sockaddr_in *addr, socklen_t len)
{
    packet msg;
    msg.type = SLEEP_STATUS_TYPE;
    msg.seqn = 0;
    msg.length = 0;
    msg.timestamp = time(NULL);
    msg._payload = NULL;
    msg.status = 1;

    int broadcast_enable = 1;
    setsockopt(sockfd, SOL_SOCKET, SO_BROADCAST, &broadcast_enable, sizeof(broadcast_enable));
    addr->sin_addr.s_addr = htonl(INADDR_BROADCAST);

    while (should_terminate_threads == 0)
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

void *listen_monitoring(void *args)
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
    serv_addr.sin_port = htons(PORT_MONITORING);

    if (bind(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
    {
        printf("Error on binding");
        pthread_exit(NULL);
    }

    int message_shown = 0;
    while (!should_terminate_threads)
    {
        if (!message_shown)
        {
            printf("Aguardando mudança de status...\n");
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
        if (msg.type == SLEEP_STATUS_TYPE)
        {
            char hostname[256], ip_address[16];
            getnameinfo((struct sockaddr *)&cli_addr, clilen, hostname, sizeof(hostname), NULL, 0, 0);
            inet_ntop(AF_INET, &(cli_addr.sin_addr.s_addr), ip_address, INET_ADDRSTRLEN);
            char mac_adress_participant[18];
            get_mac_address(mac_adress_participant);
            send_confirmed_status_msg(&serv_addr, manlen, mac_adress_participant, ip_address);
        }
    }
    close(sockfd);
    pthread_exit(NULL);
}

void *manager_start_monitoring_service(void *arg)
{
    if (should_terminate_threads)
    {
        return NULL;
    }
    // Listen for manager broadcast
    int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(PORT_MONITORING);
    socklen_t manager_addrlen = sizeof(addr);
    send_monitoring_msg(sockfd, &addr, manager_addrlen);
    // Loop que verifica should_terminate_threads e faz uma pausa
    while (!should_terminate_threads)
    {
        sleep(1); // Pause por um segundo (ou outro período de tempo desejado)
    }
    // Encerrar a função e fechar o socket
    close(sockfd);
    return NULL;
}

void *listen_Confirmed_monitoring(void *args)
{
    int sockfd;
    participant manager[MAX_PARTICIPANTS];
    int num_participants = 0;
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
    serv_addr.sin_port = htons(RESPONSE_PORT_MONITORING);
    bzero(&(serv_addr.sin_zero), 8);

    if (bind(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
    {
        printf("Error on binding");
        pthread_exit(NULL);
    }
    struct sockaddr_in cli_addr;
    socklen_t clilen = sizeof(cli_addr);
    while (!should_terminate_threads)
    {
        int n = recvfrom(sockfd, &msg, sizeof(msg), 0, (struct sockaddr *)&cli_addr, &clilen);
        if (n < 0)
        {
            printf("Error on recvfrom");
            pthread_exit(NULL);
        }
        if (msg.type == CONFIRMED_STATUS_TYPE)
        {
            char hostname[256], ip_address[16];
            getnameinfo((struct sockaddr *)&cli_addr, clilen, hostname, sizeof(hostname), NULL, 0, 0);
            inet_ntop(AF_INET, &(cli_addr.sin_addr.s_addr), ip_address, INET_ADDRSTRLEN);
            add_participant_noprint(hostname, ip_address, msg.mac_address, msg.status, msg.id_unique, PARTICIPANT_TIMEOUT, 0);
        }
        else if (msg.type == PROGRAM_EXIT_TYPE)
        {
            remove_participant(msg.mac_address);
        }
    }
    close(sockfd);
    pthread_exit(NULL);
}

void *exit_control(void *arg)
{
    while (!should_terminate_threads)
    {
        usleep(1000000);
        check_asleep_participant();
    }
}

void *monitor_manager_status(void *arg)
{
    while (should_terminate_threads == 0)
    {
        sleep(1); // Verifica o status do gerente a cada 1 segundo
        int manager_status = get_manager_status();
        if (manager_status == -1)
        {
            printf("O manager saiu, iniciando processo de eleição.\n");
            int new_manager = start_election();
            if (new_manager)
            {
                should_terminate_threads = 1;
            }
        }
    }
}