#include <semaphore.h>
#include "management_service.h"
#include "election_service.h"
#include "structs.h"

extern sem_t sem_update_interface;
extern participant participants[MAX_PARTICIPANTS];
extern int num_participants;
extern int should_terminate_threads;
extern uint64_t participant_id;
extern uint64_t current_manager_id;

void *user_interface_manager_thread(void *);
void *user_interface_participant_thread(void *);

void restore_terminal(void);

#define clear() printf("\033[H\033[J")
