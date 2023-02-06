#include <semaphore.h>
#include "management_service.h"
#include "structs.h"

extern sem_t sem_update_interface;
extern participant participants[MAX_PARTICIPANTS];
extern int num_participants;

void *user_interface_thread(void *);

#define clear() printf("\033[H\033[J")
