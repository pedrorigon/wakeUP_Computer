#ifndef _STRUCTS_H
#define _STRUCTS_H
#include <stdint.h>

typedef struct {
    char hostname[256];
    char ip_address[16];
    char mac_address[18];
    int status;
    int time_control; 
} participant;

typedef struct __packet{
    uint16_t type; //Tipo do pacote (p.ex. DATA | CMD)
    uint16_t seqn; //Número de sequência
    uint16_t length; //Comprimento do payload
    uint16_t timestamp; // Timestamp do dado
    char mac_address[18];
    int status;
    const char* _payload; //Dados da mensagem
    int time_control; 
} packet;

#endif
