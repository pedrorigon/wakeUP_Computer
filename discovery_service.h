#ifndef _DISCOVERY_SERVICE_H
#define _DISCOVERY_SERVICE_H

#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <netdb.h>
#include "structs.h"
#include <pthread.h>
#include <arpa/inet.h>
#include <sys/ioctl.h>
#include <net/if.h>


void send_discovery_msg(int sockfd, struct sockaddr_in*addr, socklen_t len);
void* listen_discovery(void* args);
void participant_start();
void get_mac_address(char* mac_address);
void* wait_for_response(void* args);
#endif