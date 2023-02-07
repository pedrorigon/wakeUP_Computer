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

#endif
