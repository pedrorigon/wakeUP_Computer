
#include "structs.h"
#include "management_service.h"
#include "discovery_service.h"
#include "monitoring_service.h"

participant participants[MAX_PARTICIPANTS];
int num_participants = 0;
pthread_mutex_t participants_mutex = PTHREAD_MUTEX_INITIALIZER;


int add_participant(char* hostname, char* ip_address, char* mac_address, int status) {
    pthread_mutex_lock(&participants_mutex);
    int atualizou_dados = 0;
    int index = find_participant(mac_address);
    if (index != -1) {
        // Se o participante já estiver na tabela --> atualiza as informações
        if(participants[index].status != status){
            strcpy(participants[index].ip_address, ip_address);
            strcpy(participants[index].mac_address, mac_address);
            participants[index].status = status;
            print_participants();
        }else{
            atualizou_dados = 1;
        }
    } else {
        if (num_participants < MAX_PARTICIPANTS) {
            // Se o participante ainda não estiver na tabela --> adiciona
            strcpy(participants[num_participants].hostname, hostname);
            strcpy(participants[num_participants].ip_address, ip_address);
            strcpy(participants[num_participants].mac_address, mac_address);
            participants[num_participants].status = status;
            num_participants++;
            print_participants();
        } else {
            printf("Error: Maximum number of participants reached.\n");
        }
    }
    //print_participants();
    pthread_mutex_unlock(&participants_mutex);
    return atualizou_dados;
}

void add_participant_noprint(char* hostname, char* ip_address, char* mac_address, int status) {
    pthread_mutex_lock(&participants_mutex);
    int index = find_participant(mac_address);
    if (index != -1) {
        // Se o participante já estiver na tabela --> atualiza as informações
        strcpy(participants[index].ip_address, ip_address);
        strcpy(participants[index].mac_address, mac_address);
        participants[index].status = status;
        
    } else {
        if (num_participants < MAX_PARTICIPANTS) {
            // Se o participante ainda não estiver na tabela --> adiciona
            strcpy(participants[num_participants].hostname, hostname);
            strcpy(participants[num_participants].ip_address, ip_address);
            strcpy(participants[num_participants].mac_address, mac_address);
            participants[num_participants].status = status;
            num_participants++;
        } else {
            printf("Error: Maximum number of participants reached.\n");
        }
    }
    //print_participants();
    pthread_mutex_unlock(&participants_mutex);
}

void remove_participant(char* mac_address) {
    pthread_mutex_lock(&participants_mutex); // lock --> exclusão mútua

    int index = find_participant(mac_address);
    if (index != -1) {
        for (int i = index; i < num_participants - 1; i++) {
            participants[i] = participants[i + 1];
        }
        num_participants--;
        print_participants();
    } else {
        printf("Error: Participant not found in table.\n");
    }
    pthread_mutex_unlock(&participants_mutex); // libera lock
}

void update_participant_status(char* mac_address, int status) {
    pthread_mutex_lock(&participants_mutex);
    int index = find_participant(mac_address);
    if (index != -1) {
        participants[index].status = status;
    } else {
        printf("Error: Participant not found in table.\n");
    }
    pthread_mutex_unlock(&participants_mutex);
}

int find_participant(char* mac_address) {
    for (int i = 0; i < num_participants; i++) {
        if (strcmp(participants[i].mac_address, mac_address) == 0) {
            return i;
        }
    }
    return -1;
}

void print_participants() {
    printf("LISTA DE PARTICIPANTES: \n");
    printf("------------------------------------------------\n");
    for (int i = 0; i < num_participants; i++) {
        printf("PARTICIPANTE: %d \n", i+1);
        printf("Hostname: %s\n", participants[i].hostname);
        printf("IP address: %s\n", participants[i].ip_address);
        printf("MAC address: %s\n", participants[i].mac_address);
        if (participants[i].status == 1){
            printf("Status: awaken\n");
        }else{
            printf("Status: asleep\n");
        }
        printf("------------------------------------------------\n");
        //printf("\n");
        //printf("Status: %d\n", participants[i].status);
    }
    printf("\n\n");
    
}

int get_participant_status(char* mac_address) {
    pthread_mutex_lock(&participants_mutex);
    int status = -1;
    int index = find_participant(mac_address);
    if (index != -1) {
        status = participants[index].status;
    } else {
        printf("Error: Participant not found in table.\n");
    }
    pthread_mutex_unlock(&participants_mutex);
    return status;
}