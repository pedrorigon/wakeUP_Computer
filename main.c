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
#include "structs.h"
#include "management_service.h"
#include "discovery_service.h"
#include "monitoring_service.h"

int main(int argc, char *argv[])
{
    arte_inicial();
    int manager = 0;
    if (argc > 1 && strcmp(argv[1], "manager") == 0)
        manager = 1;

    if (manager)
    {
        pthread_t discovery_thread;
        pthread_t monitoring_thread;
        pthread_t monitoring_confirmed_thread;
        pthread_t exit_participants_control;

        int rc = pthread_create(&discovery_thread, NULL, listen_discovery, NULL);
        if (rc)
        {
            printf("Error creating listen_discovery thread\n");
        }
        int ac = pthread_create(&monitoring_thread, NULL, manager_start_monitoring_service, NULL);
        if (ac)
        {
            printf("Error creating listen_discovery thread\n");
        }
        int bc = pthread_create(&monitoring_confirmed_thread, NULL, listen_Confirmed_monitoring, NULL);
        if (bc)
        {
            printf("Error creating listen_discovery thread\n");
        }
        int kc = pthread_create(&exit_participants_control, NULL, exit_control, NULL);
        if (kc)
        {
            printf("Error creating listen_discovery thread\n");
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
        int dc = pthread_create(&confirmed_thread, NULL, listen_Confirmed, NULL);
        if (dc)
        {
            printf("Error creating listen_discovery thread\n");
        }
        int lc = pthread_create(&msg_discovery_thread, NULL, participant_start, NULL);
        if (lc)
        {
            printf("Error creating listen_discovery thread\n");
        }
        int ec = pthread_create(&listen_monitoring_thread, NULL, listen_monitoring, NULL);
        if (ec)
        {
            printf("Error creating listen_discovery thread\n");
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

void arte_inicial(void)
{
    printf(" ----------------------------------------------------------------------------- \n");
    printf(" |      _____ _                    _____            _             _          |             \n");
    printf(" |     / ____| |                  / ____|          | |           | |         |              \n");
    printf(" |    | (___ | | ___  ___ _ __   | |     ___  _ __ | |_ _ __ ___ | |         |                 \n");
    printf(" |     \\___ \\| |/ _ \\/ _ \\ '_ \\  | |    / _ \\| '_ \\| __| '__/ _ \\| |         |                   \n");
    printf(" |     ____) | |  __/  __/ |_) | | |___| (_) | | | | |_| | | (_) | |         |                   \n");
    printf(" |    |_____/|_|\\___|\\___| .__/   \\_____\\___/|_| |_|\\__|_|  \\___/|_|         |                   \n");
    printf(" |                       | |                                                 |\n");
    printf(" |                       |_|                                                 | \n");
    printf(" ----------------------------------------------------------------------------- \n");

} 