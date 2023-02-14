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
#define PARTICIPANT_TIMEOUT 5

int add_participant(char* hostname, char* ip_address, char* mac_address, int status, int time_control);
void remove_participant(char* mac_address);
//void remove_participants(char **mac_addresses, int num_macs);
void update_participant_status(char* mac_address, int status);
int find_participant(char* mac_address);
int find_participant_by_hostname(char *hostname);
void print_participants();
void add_participant_noprint(char *hostname, char *ip_address, char *mac_address, int status, int time_control);
int get_participant_status(char* mac_address);
void check_asleep_participant();

