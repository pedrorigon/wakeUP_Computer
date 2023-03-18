#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
#include <pthread.h>
#include "election_service.h"
#include "management_service.h"
#include "discovery_service.h"
#include "monitoring_service.h"
#include "structs.h"

uint64_t participant_id;
uint64_t current_manager_id;

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

// Função que inicia a eleição
int start_election()
{
    int became_leader = 0;

    for (int i = 0; i < num_participants; i++)
    {
        if (participants[i].unique_id > participant_id)
        {
            // Envie mensagem de eleição para processos com ID maior
            send_election_message(participants[i]);
        }
    }

    // Aguarde respostas de outros processos
    int response_timeout = RESPONSE_TIMEOUT; // Defina um limite de tempo adequado
    int responses_received = wait_for_responses(response_timeout);

    if (responses_received == 0)
    {
        announce_victory();
        update_manager(participant_id);
        // current_manager_id = 1;
        became_leader = 1;
    }

    return became_leader;
}

int wait_for_responses(int response_timeout)
{
    int responses_received = 0;
    time_t start_time = time(NULL);
    int sockfd = socket(AF_INET, SOCK_DGRAM, 0);

    while (difftime(time(NULL), start_time) < response_timeout)
    {
        packet msg;
        struct sockaddr_in cli_addr;
        socklen_t clilen = sizeof(cli_addr);

        int n = recvfrom(sockfd, &msg, sizeof(msg), MSG_DONTWAIT, (struct sockaddr *)&cli_addr, &clilen);
        if (n > 0)
        {
            if (msg.type == ELECTION_RESPONSE_TYPE)
            {
                responses_received++;
            }
        }

        usleep(100000); // Espera 100ms antes de verificar novamente
    }

    return responses_received;
}

// Função que responde a uma eleição iniciada por outro processo
void respond_election(char mac_address[18], char ip_address[16], char hostname[256], int type)
{
    packet msg;
    msg.type = type;
    memcpy(msg.mac_address, mac_address, sizeof(msg.mac_address));

    struct sockaddr_in serv_addr;
    memset((char *)&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(RESPONSE_PORT);
    if (inet_aton(ip_address, &serv_addr.sin_addr) == 0)
    {
        printf("Invalid IP address: %s", ip_address);
        return;
    }

    int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0)
    {
        printf("Error opening socket");
        return;
    }

    int n = sendto(sockfd, &msg, sizeof(msg), 0, (struct sockaddr *)&serv_addr, sizeof(serv_addr));
    if (n < 0)
    {
        printf("Error sending message");
        return;
    }

    printf("Mensagem de vitória enviada para %s (%s)\n", hostname, ip_address);

    close(sockfd);
}

// Função que anuncia a vitória de uma eleição
void announce_victory()
{
    for (int i = 0; i < num_participants; i++)
    {
        if (participants[i].unique_id != participant_id)
        {
            // Envie uma mensagem de resposta de eleição para todos os outros processos
            respond_election(participants[i].mac_address, participants[i].ip_address, participants[i].hostname, ELECTION_RESPONSE_TYPE);
        }
    }
    update_manager(participant_id);
    // Chame a função become_manager() para iniciar as threads do manager
    // become_manager();
}

void *election_listener(void *arg)
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
    serv_addr.sin_port = htons(PORT_ELECTION);

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
            printf("Aguardando mensagens de eleição...\n");
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

        int sender_index = find_participant(msg.mac_address);
        if (sender_index == -1)
        {
            printf("Participante desconhecido enviou uma mensagem de eleição.");
            continue;
        }

        if (msg.type == ELECTION_TYPE)
        {
            printf("Mensagem de eleição recebida de %s\n", participants[sender_index].hostname);

            char sender_mac_address[18];
            strcpy(sender_mac_address, participants[sender_index].mac_address);

            char sender_ip_address[16];
            strcpy(sender_ip_address, participants[sender_index].ip_address);

            char sender_hostname[256];
            strcpy(sender_hostname, participants[sender_index].hostname);

            respond_election(sender_mac_address, sender_ip_address, sender_hostname, VICTORY_TYPE);
        }
        else if (msg.type == VICTORY_TYPE)
        {
            printf("Mensagem de vitória recebida de %s\n", participants[sender_index].hostname);
            update_manager(participants[sender_index].unique_id);
        }
    }
    close(sockfd);
    pthread_exit(NULL);
}

/*Adicione a lógica para iniciar a eleição quando o manager falhar ou for colocado para dormir.
 Isso dependerá da lógica do seu programa, mas geralmente você pode chamar a função start_election()
 em um callback ou em uma thread que esteja monitorando o status do manager.*/

void send_election_message(participant receiver)
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
    serv_addr.sin_port = htons(RESPONSE_PORT_ELECTION);

    if (inet_aton(receiver.ip_address, &serv_addr.sin_addr) == 0)
    {
        perror("inet_aton");
        exit(EXIT_FAILURE);
    }

    packet msg;
    msg.type = ELECTION_TYPE; // Modifique o tipo de mensagem aqui
    memcpy(msg.mac_address, receiver.mac_address, 18);
    msg.status = receiver.status;
    msg._payload = NULL;
    msg.time_control = receiver.time_control;

    sendto(sockfd, (const void *)&msg, sizeof(msg), 0, (const struct sockaddr *)&serv_addr, sizeof(serv_addr));
    close(sockfd);
}