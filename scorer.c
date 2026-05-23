#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>

#define STRING_SIZE 50
#define MAX_SIZE 256

typedef struct {
    int id;
    char inspector[STRING_SIZE];
    double lat;
    double lon;
    char category[STRING_SIZE];
    int severity;
    time_t timestamp;
    char description[MAX_SIZE];
} Report;

struct InspectorScore {
    char name[STRING_SIZE];
    int score;
};

int main(int argc, char **argv) {
    if (argc < 2) return 1;
    char *district = argv[1];

    char filepath[256];
    snprintf(filepath, sizeof(filepath), "%s/reports.dat", district);

    int fd = open(filepath, O_RDONLY);
    if (fd == -1) {
        printf("District: %s | Fara rapoarte\n", district);
        return 0;
    }

    struct InspectorScore scores[100];
    int count = 0;
    Report r;

    while (read(fd, &r, sizeof(Report)) == sizeof(Report)) {
        int found = 0;
        for (int i = 0; i < count; i++) {
            if (strcmp(scores[i].name, r.inspector) == 0) {
                scores[i].score += r.severity;
                found = 1;
                break;
            }
        }
        if (!found && count < 100) {
            strcpy(scores[count].name, r.inspector);
            scores[count].score = r.severity;
            count++;
        }
    }
    close(fd);

    for (int i = 0; i < count; i++) {
        printf("District: %s | Inspector: %s | Scor: %d\n", district, scores[i].name, scores[i].score);
    }

    return 0;
}