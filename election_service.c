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

uint64_t participant_id;
uint64_t current_manager_id;
int election_in_progress;

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
    election_in_progress = 1;
    printf("está rolando eleição: %d.\n", election_in_progress);
    int became_leader = 0;
    int num_responses = 0;

    for (int i = 0; i < num_participants; i++)
    {
        if (participants[i].unique_id > participant_id)
        {
            // Envia mensagem de eleição para processos com ID maior
            send_election_message(participants[i]);
        }
    }

    // Aguarde respostas de outros processos
    int response_timeout = RESPONSE_TIMEOUT;
    num_responses = wait_for_responses(response_timeout);

    if (num_responses == 0)
    {
        announce_victory();

        // Aguarda confirmações de todos os outros processos por um tempo limitado
        int confirmations_received = 0;
        time_t start_time = time(NULL);
        while (difftime(time(NULL), start_time) < RESPONSE_TIMEOUT)
        {
            confirmations_received = wait_for_confirmations(RESPONSE_TIMEOUT);

            if (confirmations_received == num_participants - 1)
            {
                update_manager(participant_id);
                became_leader = 1;
                break;
            }
        }

        if (!became_leader)
        {
            update_manager(participant_id);
            became_leader = 1;
        }
    }
    printf("ELEIÇÃO VAI ACABAR AGORA: %d.\n", election_in_progress);
    election_in_progress = 0;
    printf("está rolando eleição: %d.\n", election_in_progress);
    return became_leader;
}

int wait_for_responses(int response_timeout)
{
    int responses_received = 0;
    time_t start_time = time(NULL);
    int sockfd = socket(AF_INET, SOCK_DGRAM, 0);

    // Definir a estrutura sockaddr_in para ouvir na porta específica
    struct sockaddr_in serv_addr;
    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(RESPONSE_PORT_ELECTION);
    serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);

    // Vincular o socket à porta específica
    if (bind(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
    {
        perror("bind");
        exit(EXIT_FAILURE);
    }

    // Configurar o socket para ser não bloqueante
    int flags = fcntl(sockfd, F_GETFL, 0);
    fcntl(sockfd, F_SETFL, flags | O_NONBLOCK);

    while (difftime(time(NULL), start_time) < response_timeout)
    {
        packet msg;
        struct sockaddr_in cli_addr;
        socklen_t clilen = sizeof(cli_addr);

        int n = recvfrom(sockfd, &msg, sizeof(msg), 0, (struct sockaddr *)&cli_addr, &clilen);
        if (n > 0)
        {
            if (msg.type == VICTORY_TYPE)
            {
                responses_received++;
            }
        }

        usleep(100000); // Espera 100ms antes de verificar novamente
    }

    close(sockfd);
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
    serv_addr.sin_port = htons(RESPONSE_PORT_ELECTION);
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
            // Envie uma mensagem de confirmação de liderança para todos os outros processos
            respond_election(participants[i].mac_address, participants[i].ip_address, participants[i].hostname, CONFIRMATION_ELECTION_TYPE);
        }
    }
}

int wait_for_confirmations(int response_timeout)
{
    int confirmations_received = 0;
    time_t start_time = time(NULL);
    int sockfd = socket(AF_INET, SOCK_DGRAM, 0);

    // Definir a estrutura sockaddr_in para ouvir na porta específica
    struct sockaddr_in serv_addr;
    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(RESPONSE_PORT_ELECTION);
    serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);

    // Vincular o socket à porta específica
    if (bind(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
    {
        perror("bind");
        exit(EXIT_FAILURE);
    }

    // Configurar o socket para ser não bloqueante
    int flags = fcntl(sockfd, F_GETFL, 0);
    fcntl(sockfd, F_SETFL, flags | O_NONBLOCK);

    while (difftime(time(NULL), start_time) < response_timeout)
    {
        packet msg;
        struct sockaddr_in cli_addr;
        socklen_t clilen = sizeof(cli_addr);

        int n = recvfrom(sockfd, &msg, sizeof(msg), 0, (struct sockaddr *)&cli_addr, &clilen);
        if (n > 0)
        {
            if (msg.type == CONFIRMATION_ELECTION_TYPE)
            {
                confirmations_received++;
            }
        }

        usleep(100000); // Espera 100ms antes de verificar novamente
    }

    close(sockfd);
    return confirmations_received;
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

        // Verifique se a mensagem é do próprio processo
        if (participants[sender_index].unique_id == participant_id)
        {
            continue;
        }

        if (msg.type == ELECTION_TYPE)
        {
            if (!election_in_progress)
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
        }
        else if (msg.type == VICTORY_TYPE)
        {
            if (participants[sender_index].unique_id > participant_id)
            {
                printf("Mensagem de vitória recebida de %s\n", participants[sender_index].hostname);
                update_manager(participants[sender_index].unique_id);

                // Adicione o código para enviar uma mensagem de confirmação
                respond_election(participants[sender_index].mac_address, participants[sender_index].ip_address, participants[sender_index].hostname, CONFIRMATION_ELECTION_TYPE);
            }
            else
            {
                printf("Ignorando mensagem de vitória de %s (ID menor ou igual)\n", participants[sender_index].hostname);
            }
        }
    }
    close(sockfd);
    pthread_exit(NULL);
}

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
    serv_addr.sin_port = htons(PORT_ELECTION);

    if (inet_aton(receiver.ip_address, &serv_addr.sin_addr) == 0)
    {
        perror("inet_aton");
        exit(EXIT_FAILURE);
    }

    packet msg;
    msg.type = ELECTION_TYPE;
    memcpy(msg.mac_address, receiver.mac_address, 18);
    msg.status = receiver.status;
    msg._payload = NULL;
    msg.time_control = receiver.time_control;

    sendto(sockfd, (const void *)&msg, sizeof(msg), 0, (const struct sockaddr *)&serv_addr, sizeof(serv_addr));
    close(sockfd);
}

void check_for_manager(int *found_manager)
{
    printf("Procurando por um manager...\n");

    int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0)
    {
        printf("Erro ao criar o socket.\n");
        *found_manager = 0; // não encontrou nenhum manager
        return;
    }

    struct sockaddr_in serv_addr;
    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(CHECK_MANAGER_PORT);
    serv_addr.sin_addr.s_addr = INADDR_BROADCAST;

    int broadcast_permission = 1;
    if (setsockopt(sockfd, SOL_SOCKET, SO_BROADCAST, &broadcast_permission, sizeof(broadcast_permission)) < 0)
    {
        printf("Erro ao configurar o socket para broadcast.\n");
        *found_manager = 0; // não encontrou nenhum manager
        return;
    }

    struct timeval timeout;
    timeout.tv_sec = 5;
    timeout.tv_usec = 0;
    if (setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout)) < 0)
    {
        printf("Erro ao configurar o timeout do socket.\n");
        *found_manager = 0; // não encontrou nenhum manager
        return;
    }

    packet msg;
    memset(&msg, 0, sizeof(msg));
    msg.type = MANAGER_CHECK_TYPE;

    int n = sendto(sockfd, &msg, sizeof(msg), 0, (struct sockaddr *)&serv_addr, sizeof(serv_addr));
    if (n < 0)
    {
        printf("Erro ao enviar mensagem.\n");
        *found_manager = 0; // não encontrou nenhum manager
        return;
    }

    struct sockaddr_in cli_addr;
    socklen_t clilen = sizeof(cli_addr);
    packet response;

    memset(&response, 0, sizeof(response));
    n = recvfrom(sockfd, &response, sizeof(response), 0, (struct sockaddr *)&cli_addr, &clilen);
    if (n > 0 && response.type == MANAGER_RESPONSE_CHECK_TYPE)
    {
        printf("Manager encontrado!.\n");
        *found_manager = 1; // encontrou um manager
    }
    else
    {
        *found_manager = 0; // não encontrou nenhum manager
    }

    close(sockfd);
}

void *listen_manager_check(void *args)
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
    serv_addr.sin_port = htons(CHECK_MANAGER_PORT);

    if (bind(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
    {
        printf("Error on binding");
        pthread_exit(NULL);
    }

    while (!should_terminate_threads) // Modifique o loop para verificar a variável should_terminate_threads
    {
        struct sockaddr_in cli_addr;
        socklen_t clilen = sizeof(cli_addr);

        // Receive message
        int n = recvfrom(sockfd, &msg, sizeof(msg), 0, (struct sockaddr *)&cli_addr, &clilen);
        if (n < 0)
        {
            printf("Error on recvfrom");
            continue;
        }

        if (msg.type == MANAGER_CHECK_TYPE)
        {
            // This is a manager check request, so respond with our status
            packet response;
            response.type = MANAGER_RESPONSE_CHECK_TYPE;
            // Send response
            sendto(sockfd, &response, sizeof(response), 0, (struct sockaddr *)&cli_addr, clilen);
        }
    }

    pthread_exit(NULL);
}

void random_sleep()
{
    srand(time(NULL));                  // usa o tempo atual como semente
    int random_number = rand() % 4 + 2; // gera um número aleatório entre 2 e 4 segundos
    sleep(random_number);               // aguarda o número de segundos gerado aleatoriamente
}

int participant_decision()
{
    int found_manager = 0;
    // srand(time(NULL) ^ (participant_id << 16));

    check_for_manager(&found_manager);

    if (!found_manager)
    {
        printf("Manager não encontrado, verificando se há uma eleição em andamento.\n");
        int found_manager = 0;
        srand(time(NULL) ^ (participant_id << 16));

        if (election_in_progress == 0)
        {
            int random_wait = rand() % 10; // Gera um número aleatório entre 0 e 9
            sleep(random_wait);            // Aguarda um período de tempo aleatório antes de iniciar a eleição
            // check_for_manager(&found_manager); // Aguarda um período de tempo aleatório antes de iniciar a eleição

            // Verifica novamente se há uma eleição ativa e aguarda o término
            int loop_counter = 0;
            const int max_loop_count = 3;
            while (election_in_progress && loop_counter < max_loop_count)
            {
                sleep(3);
                // sleep(random_wait);
                loop_counter++; // Incrementa o contador a cada iteração
            }

            check_for_manager(&found_manager); // Verifica novamente se há um manager

            if (!found_manager) // Se ainda não há um manager
            {
                printf("Nenhuma eleição em andamento, iniciando eleição.\n");
                printf("A VARAIVEL ELECTION IN PROGRESS VALE X: %d\n", election_in_progress);
                if (election_in_progress == 0)
                {
                    int became_manager = start_election();
                    if (became_manager)
                    {
                        return 1; // Retorna 1 para indicar que o processo será iniciado como manager
                    }
                }
                else
                {
                    // Espera um tempo aleatório antes de reiniciar a função participant_decision()
                    int random_wait = rand() % 10;
                    sleep(random_wait);
                    return participant_decision();
                }
            }
        }
        else
        {
            // O código restante é o mesmo que antes
        }
    }
    else
    {
        printf("Manager encontrado, iniciando como participante.\n");
        return 0; // Retorna 0 para indicar que o processo será iniciado como participante
    }

    return 0; // Retorna 0 para indicar que o processo será iniciado como participante
}

void *send_election_active_thread(void *arg)
{
    while (1)
    {
        if (election_in_progress)
        {
            send_election_active_message();
            sleep(5);
        }
        else
        {
            // Aguarda a eleição começar novamente
            while (!election_in_progress)
            {
                sleep(1); // Aguarda 1 segundo antes de verificar novamente
            }
        }
    }
}
void send_election_active_message()
{
    int sockfd;
    struct sockaddr_in serv_addr;
    int broadcast_permission = 1; // Adicione esta linha

    if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
    {
        perror("socket");
        exit(EXIT_FAILURE);
    }

    // Adicione esta parte para habilitar a opção SO_BROADCAST no socket
    if (setsockopt(sockfd, SOL_SOCKET, SO_BROADCAST, (void *)&broadcast_permission, sizeof(broadcast_permission)) < 0)
    {
        perror("setsockopt");
        exit(EXIT_FAILURE);
    }

    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(ELECTION_ACTIVE_PORT);
    serv_addr.sin_addr.s_addr = htonl(INADDR_BROADCAST);

    packet msg;
    msg.type = ELECTION_ACTIVE_TYPE;
    printf("participant_id2: %lu \n", participant_id);
    // memcpy(&msg.mac_address, &participant_id, sizeof(uint64_t));

    if (sendto(sockfd, (const void *)&msg, sizeof(msg), 0, (const struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
    {
        perror("sendto");
        exit(EXIT_FAILURE);
    }

    close(sockfd);
}

void *election_active_listener(void *arg)
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
    serv_addr.sin_port = htons(ELECTION_ACTIVE_PORT);

    if (bind(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
    {
        printf("Error on binding");
        pthread_exit(NULL);
    }

    election_in_progress = 0;
    while (!should_terminate_threads)
    {
        struct sockaddr_in cli_addr;
        socklen_t clilen = sizeof(cli_addr);
        // election_in_progress = 0;
        //  Receive message
        int n = recvfrom(sockfd, &msg, sizeof(msg), 0, (struct sockaddr *)&cli_addr, &clilen);
        if (n < 0)
        {
            printf("Error on recvfrom");
            pthread_exit(NULL);
        }

        if (msg.type == ELECTION_ACTIVE_TYPE)
        {
            printf("Mensagem de eleição ativa recebida.\n");
            // printf("esta havendo eleição entendeu Bre booooooor\n");
            // printf("esta havendo eleição entendeu Bre booooooor\n");
            // printf("esta havendo eleição entendeu Bre booooooor\n");
            election_in_progress = 1;
        }
    }
    close(sockfd);
    pthread_exit(NULL);
}

void *send_duplicate_manager_messages(void *arg)
{
    int sockfd;
    struct sockaddr_in serv_addr;
    socklen_t servlen = sizeof(serv_addr);
    packet msg;

    msg.type = MANAGER_DUPLICATE_TYPE;
    msg.id_unique = participant_id;

    // Create socket
    if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) == -1)
    {
        printf("Error opening socket");
        pthread_exit(NULL);
    }

    // Set up destination address
    memset((char *)&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = htonl(INADDR_BROADCAST);
    serv_addr.sin_port = htons(MANAGER_DUPLICATE_PORT);

    // Enable broadcast
    int broadcast = 1;
    if (setsockopt(sockfd, SOL_SOCKET, SO_BROADCAST, &broadcast, sizeof(broadcast)) == -1)
    {
        printf("Error enabling broadcast");
        pthread_exit(NULL);
    }

    while (!should_terminate_threads)
    {
        // Send message
        if (sendto(sockfd, &msg, sizeof(msg), 0, (struct sockaddr *)&serv_addr, servlen) == -1)
        {
            printf("Error sending message");
            pthread_exit(NULL);
        }

        sleep(2); // Wait for 2 seconds before sending the next message
    }

    close(sockfd);
    pthread_exit(NULL);
}

void *listen_duplicate_manager_messages(void *arg)
{
    int sockfd;
    struct sockaddr_in serv_addr;
    socklen_t clilen;
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
    serv_addr.sin_port = htons(MANAGER_DUPLICATE_PORT);

    if (bind(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
    {
        printf("Error on binding");
        pthread_exit(NULL);
    }

    while (!should_terminate_threads)
    {
        struct sockaddr_in cli_addr;
        clilen = sizeof(cli_addr);

        // Receive message
        int n = recvfrom(sockfd, &msg, sizeof(msg), 0, (struct sockaddr *)&cli_addr, &clilen);
        if (n < 0)
        {
            printf("Error on recvfrom");
            pthread_exit(NULL);
        }

        if (msg.type == MANAGER_DUPLICATE_TYPE)
        {
            printf("Mensagem de gerente duplicado recebida.\n");

            // Compare participant IDs
            if (participant_id >= msg.id_unique)
            {
                // This manager should continue running
                printf("Este gerente continua em execução.\n");
            }
            else if (participant_id < msg.id_unique)
            {
                // This manager should restart
                printf("Este gerente deve reiniciar.\n");
                restart_program(); // Implement this function to restart the program
            }
            // If participant_id == msg.id_unique, do nothing (should not happen in normal circumstances)
        }
    }

    close(sockfd);
    pthread_exit(NULL);
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
