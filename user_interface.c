
#include "ANSI-color-codes.h"
#include <limits.h>
#include <libgen.h>
#include "user_interface.h"
#include "election_service.h"
#include "wakeonlan.h"

sem_t sem_update_interface;

/* https://stackoverflow.com/questions/2984307/how-to-handle-key-pressed-in-a-linux-console-in-c */
#include <sys/ioctl.h>
#include <termios.h>

void setup_async_terminal(void)
{
    struct termios term;
    tcgetattr(0, &term);
    term.c_lflag &= ~(ICANON | ECHO); // Disable echo as well
    tcsetattr(0, TCSANOW, &term);
}

void restore_terminal(void)
{
    struct termios term;
    tcgetattr(0, &term);
    term.c_lflag |= ICANON | ECHO;
    tcsetattr(0, TCSANOW, &term);
}

int kbhit()
{
    int byteswaiting;
    ioctl(0, FIONREAD, &byteswaiting);
    return byteswaiting > 0;
}

int get_hit()
{
    int buf = 0;
    read(0, &buf, 1);
    tcflush(0, TCIFLUSH);
    return buf;
}

/* ----------------------------------------- */

void arte_inicial(void)
{
    clear();
    printf(BHBLU " ----------------------------------------------------------------------------- \n");
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
    printf(BWHT "    MANAGER: %lu\n", current_manager_id);
    printf(BWHT "    MeuID: %lu\n", participant_id);
    printf(BHWHT " LISTA DE PARTICIPANTES: \n\n" reset);
    printf(" ------------------------------------------------\n\n");
    for (int i = 0; i < num_participants; i++)
    {
        if (participants[i].unique_id == current_manager_id)
        {
            printf(BWHT "    Participante: %d \n" reset, i + 1);
            printf(RED "    Sou o Manager da aplicação nesse momento!\n");
            printf(BWHT "    Hostname: %s\n", participants[i].hostname);
            // printf(BWHT "    IP address: %s\n", participants[i].ip_address); //to com dificuldade para pegar o ip address do manager
            printf(BWHT "    MAC address: %s\n", participants[i].mac_address);
            printf(BWHT "    ID: %lu\n", participants[i].unique_id);
            printf(BWHT "    TIME CONTROL: %d\n", participants[i].time_control);
        }
        else
        {
            printf(BWHT "  Participante: %d \n" reset, i + 1);
            printf("    Hostname: %s\n", participants[i].hostname);
            printf("    IP address: %s\n", participants[i].ip_address);
            printf("    MAC address: %s\n", participants[i].mac_address);
        }
        if (participants[i].status == 1)
        {
            if (participants[i].time_control > PARTICIPANT_TIMEOUT / 2)
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

void print_manager(void)
{
    if (!manager.status)
    {
        printf(YEL " Aguardando manager...\n" reset);
    }
    else
    {
        printf(GRN " Manager adquirido\n" reset);
        printf("    Hostname: %s\n", manager.hostname);
        printf("    IP address: %s\n", manager.ip_address);
        printf("    MAC address: %s\n", manager.mac_address);
        if (manager.status == 1)
        {
            if (manager.time_control > PARTICIPANT_TIMEOUT / 2)
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
}

void *user_interface_manager_thread(void *args)
{
    if (sem_init(&sem_update_interface, 0, 1) != 0)
    {
        printf(RED "Error initializing UI semaphore!\n" reset);
    }

    while (!should_terminate_threads)
    {
        setup_async_terminal();
        clear();
        arte_inicial();
        print_participants();
        puts(" Pressione c para entrar em modo de comando");
        while (!kbhit() && sem_trywait(&sem_update_interface))
            ;
        if (!kbhit())
            continue; // Se saiu por causa do semáforo, segue em frente
        int key = get_hit();
        if (key == 'c')
        {
            restore_terminal();
            clear();
            arte_inicial();
            puts(BWHT " Modo comando (WAKEUP <hostname>, EXIT)" reset);
            print_participants();
            int valid_command = 0;
            char buffer[255] = {0};
            while (!valid_command)
            {
                printf(" > ");
                memset(buffer, 0, sizeof(buffer));
                fgets(buffer, sizeof(buffer), stdin);

                // Lida com caso do usuário pressionar CTRL-D
                if (feof(stdin))
                {
                    clearerr(stdin);
                    break;
                };
                buffer[strlen(buffer) - 1] = '\0';
                if (strcmp(buffer, "EXIT") == 0)
                {
                    exit(0);
                }
                else if (strncmp(buffer, "WAKEUP ", sizeof("WAKEUP ") - 1) == 0)
                {
                    char *hostname = buffer + sizeof("WAKEUP"); // eu odeio ponteiros mas aqui eles são úteis
                    char buffer[255] = {0};
                    int index = find_participant_by_hostname(hostname);
                    if (index == -1)
                    {
                        puts(RED " Participante não encontrado!" reset);
                        continue;
                    }
                    else
                    {
                        printf("Acordando %s\n", hostname);
                        int rc = wakeonlan(participants[index].mac_address);
                        if (rc != 00)
                        {
                            puts(RED "Erro ao usar wakeonlan!" reset);
                            continue;
                        }
                        else
                        {
                            valid_command = 1;
                            puts(GRN "Computador acordado com sucesso!" reset);
                            usleep(3 * 1000 * 1000);
                        }
                    }
                }
                else
                {
                    puts(RED " Comando invalido!" reset);
                }
            };
        }
    }
    return NULL;
}

void *user_interface_participant_thread(void *args)
{
    if (sem_init(&sem_update_interface, 0, 1) != 0)
    {
        printf(RED "Error initializing UI semaphore!\n" reset);
    }

    while (!should_terminate_threads)
    {
        pthread_testcancel();

        setup_async_terminal();
        clear();
        arte_inicial();
        print_participants();
        print_manager();
        puts(" Pressione c para entrar em modo de comando");
        while (!kbhit() && sem_trywait(&sem_update_interface))
        {
            pthread_testcancel(); // chamada para verificar o canceamento
        }
        if (!kbhit())
            continue; // Se saiu por causa do semáforo, segue em frente
        int key = get_hit();
        if (key == 'c')
        {
            restore_terminal();
            clear();
            arte_inicial();
            puts(BWHT " Modo comando (EXIT)" reset);
            print_participants();
            print_manager();
            int valid_command = 0;
            char buffer[255] = {0};
            while (!valid_command)
            {
                pthread_testcancel();

                printf(" > ");
                memset(buffer, 0, sizeof(buffer));
                fgets(buffer, sizeof(buffer), stdin);

                // Lida com caso do usuário pressionar CTRL-D
                if (feof(stdin))
                {
                    clearerr(stdin);
                    break;
                };
                buffer[strlen(buffer) - 1] = '\0';
                if (strcmp(buffer, "EXIT") == 0)
                {
                    send_goodbye_msg();
                    exit(0);
                }
                else
                {
                    puts(RED " Comando invalido!" reset);
                }
            };
        }
    }
    return NULL;
}