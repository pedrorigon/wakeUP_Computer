#include "structs.h"
#include "discovery_service.h"
#include "management_service.h"

// Manager part
void syncrhonization_manager(void) {
    struct sockaddr_in addr;
    int sockfd = socket(AF_INET, SOCK_DGRAM, 0);

    int broadcast_enable = 1;
    setsockopt(sockfd, SOL_SOCKET, SO_BROADCAST, &broadcast_enable, sizeof(broadcast_enable));
    addr.sin_addr.s_addr = htonl(INADDR_BROADCAST);
    addr.sin_port = htons(SYNCRONIZATION_PORT);

    while (should_terminate_threads == 0)
    {
        usleep(1000000);
        int n = sendto(sockfd, participants, sizeof(participants), 0, (struct sockaddr *)&addr, sizeof(addr));
        if (n < 0)
        {
            printf("ERROR on sendto");
        }
        //puts("sending out sync table");
    }
    close(sockfd);
}

// Participant part
void syncrhonization_client(void) {
    int sockfd;
    struct sockaddr_in serv_addr;
    participant new_table[MAX_PARTICIPANTS];

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
    serv_addr.sin_port = htons(SYNCRONIZATION_PORT);
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
        int n = recvfrom(sockfd, &new_table, sizeof(new_table), 0, (struct sockaddr *)&cli_addr, &clilen);
        if (n < 0)
        {
            printf("Error on recvfrom");
            pthread_exit(NULL);
        }
        //puts("received table... syncing");
        sync_local_participants(new_table);
        
    }
    close(sockfd);
    pthread_exit(NULL);
}