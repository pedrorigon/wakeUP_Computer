#ifndef _STRUCTS_H
#define _STRUCTS_H
#include <stdint.h>

typedef struct
{
    char hostname[256];
    char ip_address[16];
    char mac_address[18];
    int status;
    uint64_t unique_id;
    int time_control;
    int is_manager;
} participant;

typedef struct __packet
{
    uint16_t type;      // Tipo do pacote (p.ex. DATA | CMD)
    uint16_t seqn;      // Número de sequência
    uint16_t length;    // Comprimento do payload
    uint16_t timestamp; // Timestamp do dado
    char mac_address[18];
    int status;
    const char *_payload; // Dados da mensagem
    int time_control;
    uint64_t id_unique;
    uint64_t election_id;
} packet;

#define MAX_ARGC 2
#define PORT 4000
#define RESPONSE_PORT 4001
#define PORT_MONITORING 4002
#define RESPONSE_PORT_MONITORING 4003
#define PORT_ELECTION 4004
#define SYNCRONIZATION_PORT 4005

#define DISCOVERY_TYPE 1
#define CONFIRMED_TYPE 2
#define SLEEP_STATUS_TYPE 3
#define CONFIRMED_STATUS_TYPE 4
#define PROGRAM_EXIT_TYPE 5


#define ELECTION_TYPE 6
#define ALIVE_TYPE 7
#define ANSWER_TYPE 8
#define VICTORY_TYPE 9


#define RESPONSE_TIMEOUT 5
#define STATUS_ASLEEP 0
#define STATUS_AWAKE 1

#endif
