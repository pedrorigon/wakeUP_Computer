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
#include "synchronization_service.h"

void sig_handler(int);

void start_manager_threads();
void start_participant_threads();
int should_terminate_threads = 0;

int main()
{
    initialize_participant_id();
    pthread_t send_election_active_tid, election_active_listener_tid;
    pthread_t election_listener_thread;

    int rc = pthread_create(&election_listener_thread, NULL, election_listener, NULL);
    if (rc)
    {
        printf("Error creating election_listener thread\n");
    }

    if (pthread_create(&send_election_active_tid, NULL, send_election_active_thread, NULL) != 0)
    {
        printf("Erro ao criar a thread send_election_active_thread.\n");
        exit(EXIT_FAILURE);
    }

    if (pthread_create(&election_active_listener_tid, NULL, election_active_listener, NULL) != 0)
    {
        printf("Erro ao criar a thread election_active_listener.\n");
        exit(EXIT_FAILURE);
    }
    int manager = participant_decision();

    // Encerra as threads
    pthread_cancel(election_listener_thread);
    pthread_cancel(send_election_active_tid);
    pthread_cancel(election_active_listener_tid);

    // Aguarda a finalização das threads
    pthread_join(election_listener_thread, NULL);
    pthread_join(send_election_active_tid, NULL);
    pthread_join(election_active_listener_tid, NULL);

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
    should_terminate_threads = 0;
    insert_manager_into_participants_table();
    pthread_t discovery_thread;
    pthread_t monitoring_thread;
    pthread_t monitoring_confirmed_thread;
    pthread_t manager_check_thread;
    pthread_t listen_duplicate_manager_thread;
    pthread_t send_duplicate_manager_thread;
    pthread_t exit_participants_control;
    pthread_t synchronization_manager_thread;

    int rc = pthread_create(&discovery_thread, NULL, listen_discovery, NULL);
    if (rc)
    {
        printf("Error creating listen_discovery thread\n");
    }
    rc = pthread_create(&manager_check_thread, NULL, listen_manager_check, NULL);
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
    if (pthread_create(&listen_duplicate_manager_thread, NULL, listen_duplicate_manager_messages, NULL) != 0)
    {
        printf("Error creating listen_duplicate_manager_messages thread");
    }

    if (pthread_create(&send_duplicate_manager_thread, NULL, send_duplicate_manager_messages, NULL) != 0)
    {
        printf("Error creating send_duplicate_manager_messages thread");
    }
    if (pthread_create(&synchronization_manager_thread, NULL, syncrhonization_manager, NULL) != 0)
    {
        printf("Error creating synchronization_manager_thread thread");
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
    rc = pthread_create(&monitor_manager_status_thread, NULL, monitor_manager_status, NULL);
    if (rc)
    {
        printf("Error creating monitor_manager_status thread\n");
    }
    if (pthread_create(&synchronization_client_thread, NULL, syncrhonization_client, NULL) != 0)
    {
        printf("Error creating synchronization_manager_thread thread");
    }
    while (!should_terminate_threads)
    {
        if (current_manager_id == participant_id)
        {
            should_terminate_threads = 1;
        }

        sleep(1);
    }

    // Encerra as threads de participante
    printf("Canceling election_listener_thread\n");
    pthread_cancel(election_listener_thread);
    printf("Canceling confirmed_thread\n");
    pthread_cancel(confirmed_thread);
    printf("Canceling msg_discovery_thread\n");
    pthread_cancel(msg_discovery_thread);
    printf("Canceling listen_monitoring_thread\n");
    pthread_cancel(listen_monitoring_thread);
    printf("Canceling user_interface_control\n");
    pthread_cancel(user_interface_control);
    printf("Canceling exit_participants_control\n");
    pthread_cancel(exit_participants_control);
    printf("Canceling monitor_manager_status_thread\n");
    pthread_cancel(monitor_manager_status_thread);

    sleep(1);
    // Aguarda a finalização das threads
    printf("Joining election_listener_thread\n");
    pthread_join(election_listener_thread, NULL);
    printf("Joining confirmed_thread\n");
    pthread_join(confirmed_thread, NULL);
    printf("Joining msg_discovery_thread\n");
    pthread_join(msg_discovery_thread, NULL);
    printf("Joining listen_monitoring_thread\n");
    pthread_join(listen_monitoring_thread, NULL);
    printf("Joining user_interface_control\n");
    pthread_join(user_interface_control, NULL);
    printf("Joining exit_participants_control\n");
    pthread_join(exit_participants_control, NULL);
    printf("Joining monitor_manager_status_thread\n");
    pthread_join(monitor_manager_status_thread, NULL);

    // Inicia as threads de gerente
    start_manager_threads();
}