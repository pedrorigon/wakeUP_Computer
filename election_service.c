#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
#include <pthread.h>
#include <time.h>
#include "election_service.h"
#include "management_service.h"
#include "discovery_service.h"
#include "monitoring_service.h"
#include "structs.h"
#include <fcntl.h>
#include <limits.h>

#define TIMEOUT_VALUE 5

uint64_t election_contested = 0;
uint64_t manager_last_alive = 0;

uint64_t generate_unique_id()
{
    char mac_address[18];
    get_mac_address(mac_address);

    unsigned int m[6];
    sscanf(mac_address, "%02x:%02x:%02x:%02x:%02x:%02x", &m[0], &m[1], &m[2], &m[3], &m[4], &m[5]);
    uint64_t unique_id = 0;
    for (int i = 0; i < 6; i++)
    {
        unique_id = (unique_id << 8) | m[i];
    }
    return unique_id;
}

// Função para inicializar o participant_id
void initialize_participant_id()
{
    participant_id = generate_unique_id();
}

void send_message(int type)
{
    int sockfd;
    struct sockaddr_in serv_addr;

    if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
    {
        perror("socket");
        exit(EXIT_FAILURE);
    }

    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(PORT_ELECTION);
    int broadcast_enable = 1;
    setsockopt(sockfd, SOL_SOCKET, SO_BROADCAST, &broadcast_enable, sizeof(broadcast_enable));
    serv_addr.sin_addr.s_addr = htonl(INADDR_BROADCAST);

    packet msg = {
        .id_unique = participant_id,
        .type = type,
    };

    sendto(sockfd, (const void *)&msg, sizeof(msg), 0, (const struct sockaddr *)&serv_addr, sizeof(serv_addr));
    close(sockfd);
}

void *election_server(void *)
{
    int sockfd;
    struct sockaddr_in serv_addr;
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
    serv_addr.sin_port = htons(PORT_ELECTION);

    if (bind(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
    {
        printf("Error on binding");
        pthread_exit(NULL);
    }

    while (1)
    {
        struct sockaddr_in cli_addr;
        socklen_t clilen = sizeof(cli_addr);

        // Receive message
        int n = recvfrom(sockfd, &msg, sizeof(msg), 0, (struct sockaddr *)&cli_addr, &clilen);
        if (n < 0)
        {
            printf("Error on recvfrom");
            pthread_exit(NULL);
        }

        switch (msg.type)
        {
        case ELECTION_TYPE:
            // Block others with smaller ids of becoming managers
            if (msg.id_unique < participant_id)
                send_message(ANSWER_TYPE);
            puts("Received election probe");
            break;
        case ALIVE_TYPE:
            // Keep track whether the manager is alive
            puts("Received keep-alive");
            manager_last_alive = time(NULL);
            if (msg.id_unique != current_manager_id)
                update_manager(current_manager_id);
            if(msg.id_unique > current_manager_id && (current_manager_id == participant_id))
                restart_program();
            break;
        case ANSWER_TYPE:
            // Our election probe was contested and we won't become managers
            puts("Received answer");
            if (msg.id_unique > participant_id)
                election_contested = 1;
            break;
        case VICTORY_TYPE:
            printf("New manager %08lx\n", current_manager_id);
            update_manager(current_manager_id);
            break;
        default:
            break;
        }
    }
    close(sockfd);
    pthread_exit(NULL);
}

// Função que inicia a eleição
int start_election()
{
    election_contested = 0;
    // Broadcast our intentions of starting an election
    send_message(ELECTION_TYPE);
    // Wait for contestation
    sleep(TIMEOUT_VALUE);
    if (election_contested)
    {
        puts("I won't be the manager...");
        return 0;
    }
    else
    {
        puts("I'm the manager!");
        update_manager(participant_id);
        send_message(VICTORY_TYPE);
        return 1;
    }
}

int participant_decision(void) {
    puts("Waiting to see whether there's a manager...");
    sleep(TIMEOUT_VALUE);
    if(current_manager_id) {
        puts("Found one!");
        return 0;
    } else {
        return start_election;
    }

}

void *monitoring_thread(void *)
{
    while (1)
    {
        if (current_manager_id == participant_id)
        {
            send_message(ALIVE_TYPE);
            update_manager(participant_id);
            puts("I'm the manager!");
        }
        else
        {
            if (time(NULL) - manager_last_alive > TIMEOUT_VALUE)
            {
                puts("My manager is dead!");
                start_election();
            }
            else
            {
                printf("My manager is %08lx and he's alive and happy!\n", current_manager_id);
            }
        }
        sleep(1);
    }
}


void restart_program()
{
    char path[PATH_MAX];
    ssize_t len = readlink("/proc/self/exe", path, sizeof(path) - 1);

    if (len == -1)
    {
        perror("Error getting program path");
        exit(EXIT_FAILURE);
    }

    // Clean sockets before execv
    size_t n = sysconf(_SC_OPEN_MAX);
    for(size_t i = 3; i < n; i++)
        close(i);

    path[len] = '\0';

    char *new_argv[MAX_ARGC];
    new_argv[0] = path;
    new_argv[1] = NULL;

    if (execv(new_argv[0], new_argv) == -1)
    {
        perror("Error restarting program");
        exit(EXIT_FAILURE);
    }
}
