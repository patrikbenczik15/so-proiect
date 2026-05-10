#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <fcntl.h>
#include <string.h>

void handle_sigint(int sig) {
    unlink(".monitor_pid");
    printf("\nmonitor oprit\n");
    exit(0);
}

void handle_sigusr1(int sig) {
    printf("Notificare: un nou raport a fost adaugat\n");
}

int main() {
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

    while (1) {
        pause();
    }

    return 0;
}