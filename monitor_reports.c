#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <fcntl.h>
#include <string.h>

void handle_sigint(int sig) {
    unlink(".monitor_pid");
    printf("STOP: Monitor oprit\n");
    fflush(stdout);
    exit(0);
}

void handle_sigusr1(int sig) {
    printf("EVENT: Un nou raport a fost adaugat!\n");
    fflush(stdout);
}

int main() {
    int pid_fd = open(".monitor_pid", O_RDONLY);
    if (pid_fd != -1) {
        char pid_buf[32] = {0};
        read(pid_fd, pid_buf, sizeof(pid_buf) - 1);
        close(pid_fd);
        int existing_pid = atoi(pid_buf);
        if (existing_pid > 0 && kill(existing_pid, 0) == 0) {
            printf("ERROR: Monitor deja ruleaza cu PID %d\n", existing_pid);
            fflush(stdout);
            return 1;
        }
    }

    struct sigaction sa_int;
    sa_int.sa_handler = handle_sigint;
    sigemptyset(&sa_int.sa_mask);
    sa_int.sa_flags = 0;
    sigaction(SIGINT, &sa_int, NULL);

    struct sigaction sa_usr1;
    sa_usr1.sa_handler = handle_sigusr1;
    sigemptyset(&sa_usr1.sa_mask);
    sa_usr1.sa_flags = 0;
    sigaction(SIGUSR1, &sa_usr1, NULL);

    int fd = open(".monitor_pid", O_CREAT | O_WRONLY | O_TRUNC, 0644);
    if (fd != -1) {
        char pid_str[32];
        int len = snprintf(pid_str, sizeof(pid_str), "%d", getpid());
        write(fd, pid_str, len);
        close(fd);
    }

    printf("START: Monitor pornit cu PID %d\n", getpid());
    fflush(stdout);

    while (1) {
        pause();
    }

    return 0;
}