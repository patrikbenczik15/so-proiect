#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <signal.h>

void cmd_start_monitor() {
    pid_t hub_mon = fork();
    if (hub_mon == 0) {
        int pipefd[2];
        pipe(pipefd);

        pid_t monitor_pid = fork();
        if (monitor_pid == 0) {
            close(pipefd[0]);
            dup2(pipefd[1], STDOUT_FILENO);
            close(pipefd[1]);
            execl("./monitor_reports", "monitor_reports", NULL);
            exit(1);
        } else if (monitor_pid > 0) {
            close(pipefd[1]);
            char buffer[256];
            int bytes_read;
            while ((bytes_read = read(pipefd[0], buffer, sizeof(buffer) - 1)) > 0) {
                buffer[bytes_read] = '\0';
                printf("\n[hub_mon a primit]: %s", buffer);
                if (strstr(buffer, "STOP:") || strstr(buffer, "ERROR:")) {
                    printf("-> Sistem: monitorul principal a oprit transmisia.\n");
                    break;
                }
            }
            close(pipefd[0]);
            waitpid(monitor_pid, NULL, 0);
            exit(0);
        }
    }
}

void cmd_calculate_scores(char **districts, int count) {
    int pipes[count][2];
    pid_t pids[count];

    for (int i = 0; i < count; i++) {
        pipe(pipes[i]);
        pids[i] = fork();

        if (pids[i] == 0) {
            close(pipes[i][0]);
            dup2(pipes[i][1], STDOUT_FILENO);
            close(pipes[i][1]);
            execl("./scorer", "scorer", districts[i], NULL);
            exit(1);
        }
    }

    printf("\n--- Raport Combinat de Workload ---\n");
    for (int i = 0; i < count; i++) {
        close(pipes[i][1]);
        char buffer[512];
        int bytes_read;
        while ((bytes_read = read(pipes[i][0], buffer, sizeof(buffer) - 1)) > 0) {
            buffer[bytes_read] = '\0';
            printf("%s", buffer);
        }
        close(pipes[i][0]);
        waitpid(pids[i], NULL, 0);
    }
    printf("-----------------------------------\n");
}

int main() {
    signal(SIGCHLD, SIG_IGN);
    char input[256];
    
    printf("City Hub pornit. Scrie comenzi ('exit' pentru a iesi).\n");

    while (1) {
        printf("city_hub> ");
        fflush(stdout);
        
        if (!fgets(input, sizeof(input), stdin)) break;

        input[strcspn(input, "\n")] = 0;
        if (strlen(input) == 0) continue;

        if (strcmp(input, "exit") == 0) {
            break;
        }

        char *token = strtok(input, " ");
        if (strcmp(token, "start_monitor") == 0) {
            cmd_start_monitor();
        } else if (strcmp(token, "calculate_scores") == 0) {
            char *districts[20];
            int count = 0;
            while ((token = strtok(NULL, " ")) != NULL) {
                districts[count++] = token;
            }
            if (count > 0) {
                cmd_calculate_scores(districts, count);
            } else {
                printf("Eroare: Trebuie specificat cel putin un district.\n");
            }
        } else {
            printf("Comanda necunoscuta.\n");
        }
    }
    
    return 0;
}