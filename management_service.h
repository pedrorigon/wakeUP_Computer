#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h> 
#include <string.h>
#include <netdb.h>
#include <pthread.h>
#define MAX_PARTICIPANTS 100
void add_participant(char* hostname, char* ip_address, char* mac_address, int status);
void remove_participant(char* hostname);
void update_participant_status(char* hostname, int status);
int find_participant(char* hostname);
void print_participants();
