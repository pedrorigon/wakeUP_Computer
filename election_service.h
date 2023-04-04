#ifndef _ELECTION_SERVICE_H
#define _ELECTION_SERVICE_H

#include "structs.h"
#include <pthread.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <stdint.h>
#include <net/if.h>
#include <time.h>
#include <limits.h>
#include "management_service.h"
#include "discovery_service.h"
#include "monitoring_service.h"
#include "structs.h"
#include <fcntl.h>
#include <limits.h>
#include <libgen.h>

extern uint64_t participant_id;
extern uint64_t current_manager_id;
extern int num_participants;
extern participant participants[MAX_PARTICIPANTS];
extern int should_terminate_threads;

void initialize_participant_id();
uint64_t generate_unique_id();
int start_election();
void *election_server(void *);
void *monitoring_thread(void *);
void restart_program();
#endif