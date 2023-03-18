#ifndef _ELECTION_SERVICE_H
#define _ELECTION_SERVICE_H

#include "structs.h"
#include <pthread.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <stdint.h>
#include <net/if.h>
#include "management_service.h"
#include "discovery_service.h"
#include "monitoring_service.h"
#include "structs.h"

extern uint64_t participant_id;
extern uint64_t current_manager_id;
extern int num_participants;
extern participant participants[MAX_PARTICIPANTS];
extern int should_terminate_threads;

void initialize_participant_id();
uint64_t generate_unique_id();
int start_election();
int wait_for_responses(int response_timeout);
void respond_election(char mac_address[18], char ip_address[16], char hostname[256], int type);
void announce_victory();
void update_manager(uint64_t new_manager_id);
void *election_listener(void *arg);
void become_manager();
void send_election_message(participant receiver);
int participant_decision();
void *listen_manager_check(void *args);
void check_for_manager(int *found_manager);

#endif