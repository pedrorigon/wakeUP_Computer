
#include "ANSI-color-codes.h"
#include "user_interface.h"

sem_t sem_update_interface;

void arte_inicial(void)
{
    clear();
    printf(BHBLU" ----------------------------------------------------------------------------- \n");
    printf(" |      _____ _                    _____            _             _          |             \n");
    printf(" |     / ____| |                  / ____|          | |           | |         |              \n");
    printf(" |    | (___ | | ___  ___ _ __   | |     ___  _ __ | |_ _ __ ___ | |         |                 \n");
    printf(" |     \\___ \\| |/ _ \\/ _ \\ '_ \\  | |    / _ \\| '_ \\| __| '__/ _ \\| |         |                   \n");
    printf(" |     ____) | |  __/  __/ |_) | | |___| (_) | | | | |_| | | (_) | |         |                   \n");
    printf(" |    |_____/|_|\\___|\\___| .__/   \\_____\\___/|_| |_|\\__|_|  \\___/|_|         |                   \n");
    printf(" |                       | |                                                 |\n");
    printf(" |                       |_|                                                 | \n");
    printf(" ----------------------------------------------------------------------------- \n\n" reset);

}

void print_participants()
{
    printf(BHWHT " LISTA DE PARTICIPANTES: \n\n" reset);
    printf(" ------------------------------------------------\n\n");
    for (int i = 0; i < num_participants; i++)
    {
        printf(BWHT "  Participante: %d \n" reset, i + 1);
        printf("    Hostname: %s\n", participants[i].hostname);
        printf("    IP address: %s\n", participants[i].ip_address);
        printf("    MAC address: %s\n", participants[i].mac_address);
        if (participants[i].status == 1)
        {
            if(participants[i].time_control > PARTICIPANT_TIMEOUT/2)
                printf("    Status: " GRN "awaken\n" reset);
            else
                printf("    Status: " YEL "timing out\n" reset);
        }
        else
        {
            
            printf("    Status: " CYN "asleep\n" reset);
        }
        printf("\n ------------------------------------------------\n\n");

    }
    printf("\n\n");
}

void print_manager(void) {
    if(!manager.status) {
        printf(YEL " Aguardando manager...\n" reset);
    } else {
        printf(GRN " Manager adquirido\n" reset);
        printf("    Hostname: %s\n", manager.hostname);
        printf("    IP address: %s\n", manager.ip_address);
        printf("    MAC address: %s\n", manager.mac_address);
    }
}

void *user_interface_manager_thread(void *args) {
    if ( sem_init(&sem_update_interface, 0, 1) != 0 )
    {
        printf(RED "Error initializing UI semaphore!\n" reset);
    }

    while(1) {
        sem_wait(&sem_update_interface);
        clear();
        arte_inicial();
        print_participants();
    }
}

void *user_interface_participant_thread(void *args) {
    if ( sem_init(&sem_update_interface, 0, 1) != 0 )
    {
        printf(RED "Error initializing UI semaphore!\n" reset);
    }
    clear();
    arte_inicial();
    print_manager();

    while(1) {
        sem_wait(&sem_update_interface);
        clear();
        arte_inicial();
        print_manager();
    }
}