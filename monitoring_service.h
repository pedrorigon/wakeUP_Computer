#ifndef _MONITORING_SERVICE_H
#define _MONITORING_SERVICE_H

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


void send_confirmed_status_msg(struct sockaddr_in *addr, socklen_t len, char mac_address[18], char ip_address[16]);
void send_monitoring_msg(int sockfd, struct sockaddr_in *addr, socklen_t len);
void *listen_monitoring(void *args);
void manager_start_monitoring_service();
void *listen_Confirmed_monitoring(void *args);
void exit_control(); 

#endif