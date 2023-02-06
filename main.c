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

void sig_handler(int);

int main(int argc, char *argv[])
{
    int manager = 0;
    if (argc > 1 && strcmp(argv[1], "manager") == 0)
        manager = 1;

    signal(SIGTERM, sig_handler);
    signal(SIGINT, sig_handler);

    if (manager)
    {
        pthread_t discovery_thread;
        pthread_t monitoring_thread;
        pthread_t monitoring_confirmed_thread;
        pthread_t exit_participants_control;
        pthread_t user_interface_control;

        int rc = pthread_create(&discovery_thread, NULL, listen_discovery, NULL);
        if (rc)
        {
            printf("Error creating listen_discovery thread\n");
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

        int ret_join = pthread_join(discovery_thread, NULL);
        if (ret_join)
        {
            printf("Error joining thread\n");
        }
    }
    else
    {

        pthread_t confirmed_thread;
        pthread_t msg_discovery_thread;
        pthread_t listen_monitoring_thread;
        pthread_t user_interface_control;

        int rc = pthread_create(&confirmed_thread, NULL, listen_Confirmed, NULL);
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

        printf("\n");

        int ret_join = pthread_join(confirmed_thread, NULL);
        if (ret_join)
        {
            printf("Error joining thread\n");
        }
        else
        {
            printf("deu join\n");
        }
    }

    return 0;
}

void sig_handler(int signum) {

  //Return type of the handler function should be void
  printf("\nInside handler function\n");
  exit(0);
}