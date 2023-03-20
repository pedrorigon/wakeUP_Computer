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
#include <ifaddrs.h>
#include "election_service.h"

extern int should_terminate_threads;

void send_goodbye_msg(void);
void send_discovery_msg(int sockfd, struct sockaddr_in *addr, socklen_t len, char mac_address[18]);
void *listen_discovery(void *args);
void *participant_start(void *arg);
void get_mac_address(char *mac_address);
void *listen_Confirmed(void *args);
void send_confirmed_msg(struct sockaddr_in *addr, socklen_t len, char mac_address[18], char ip_address[16]);
void insert_manager_into_participants_table();
char *get_local_ip_address();
#endif