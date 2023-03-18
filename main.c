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
#include <signal.h>
#include "structs.h"
#include "management_service.h"
#include "discovery_service.h"
#include "monitoring_service.h"
#include "user_interface.h"
#include "election_service.h"

void sig_handler(int);

void start_manager_threads();
void start_participant_threads();
int should_terminate_threads = 0;

int main()
{
    initialize_participant_id();
    int manager = participant_decision();

    signal(SIGTERM, sig_handler);
    signal(SIGINT, sig_handler);

    if (manager == 1)
    {
        start_manager_threads();
    }
    else
    {
        start_participant_threads();
    }

    return 0;
}

void sig_handler(int signum)
{
    send_goodbye_msg();
    printf("\nSaindo graciosamente\n");
    restore_terminal();
    exit(0);
}

void start_manager_threads()
{
    insert_manager_into_participants_table();
    pthread_t discovery_thread;
    pthread_t monitoring_thread;
    pthread_t monitoring_confirmed_thread;
    pthread_t manager_check_thread;

    int rc = pthread_create(&discovery_thread, NULL, listen_discovery, NULL);
    if (rc)
    {
        printf("Error creating listen_discovery thread\n");
    }
    rc = pthread_create(&manager_check_thread, NULL, listen_manager_check, NULL); // inicia a thread para ouvir as solicitações de verificação do status do gerenciador
    if (rc)
    {
        printf("Error creating manager_check thread\n");
    }
    rc = pthread_create(&monitoring_thread, NULL, manager_start_monitoring_service, NULL);
    if (rc)
    {
        printf("Error creating monitoring thread\n");
    }
    rc = pthread_create(&monitoring_confirmed_thread, NULL, listen_Confirmed_monitoring, NULL);
    if (rc)
    {
        printf("Error creating monitoring_confirmed thread\n");
    }
    rc = pthread_create(&exit_participants_control, NULL, exit_control, NULL);
    if (rc)
    {
        printf("Error creating exit_participants thread\n");
    }
    rc = pthread_create(&user_interface_control, NULL, user_interface_manager_thread, NULL);
    if (rc)
    {
        printf("Error creating user_interface thread\n");
    }

    pthread_join(discovery_thread, NULL);
}

void start_participant_threads()
{

    int rc = pthread_create(&election_listener_thread, NULL, election_listener, NULL);
    if (rc)
    {
        printf("Error creating election_listener thread\n");
    }

    rc = pthread_create(&monitor_manager_status_thread, NULL, monitor_manager_status, NULL);
    if (rc)
    {
        printf("Error creating monitor_manager_status thread\n");
    }
    rc = pthread_create(&confirmed_thread, NULL, listen_Confirmed, NULL);
    if (rc)
    {
        printf("Error creating listen_discovery thread\n");
    }
    rc = pthread_create(&msg_discovery_thread, NULL, participant_start, NULL);
    if (rc)
    {
        printf("Error creating listen_discovery thread\n");
    }
    rc = pthread_create(&listen_monitoring_thread, NULL, listen_monitoring, NULL);
    if (rc)
    {
        printf("Error creating listen_discovery thread\n");
    }
    rc = pthread_create(&user_interface_control, NULL, user_interface_participant_thread, NULL);
    if (rc)
    {
        printf("Error creating user_interface thread\n");
    }
    rc = pthread_create(&exit_participants_control, NULL, exit_control, NULL);
    if (rc)
    {
        printf("Error creating exit_participants thread\n");
    }

    pthread_join(confirmed_thread, NULL);
}