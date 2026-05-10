#include <dirent.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>
#include <time.h>
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

void mode_to_str(mode_t mode, char *str) {
    strcpy(str, "---------");
    if (mode & S_IRUSR) str[0] = 'r';
    if (mode & S_IWUSR) str[1] = 'w';
    if (mode & S_IXUSR) str[2] = 'x';
    if (mode & S_IRGRP) str[3] = 'r';
    if (mode & S_IWGRP) str[4] = 'w';
    if (mode & S_IXGRP) str[5] = 'x';
    if (mode & S_IROTH) str[6] = 'r';
    if (mode & S_IWOTH) str[7] = 'w';
    if (mode & S_IXOTH) str[8] = 'x';
}

int check_permission(const char *filepath, const char *role, mode_t manager_bit, mode_t inspector_bit) {
    struct stat st;
    if (stat(filepath, &st) == -1)
        return 0;

    if (strcmp(role, "manager") == 0) {
        return (st.st_mode & manager_bit) ? 1 : 0;
    } else if (strcmp(role, "inspector") == 0) {
        return (st.st_mode & inspector_bit) ? 1 : 0;
    }
    return 0;
}

int parse_condition(const char *input, char *field, char *op, char *value) {
    char temp[256];
    strncpy(temp, input, sizeof(temp));

    char *p1 = strchr(temp, ':');
    if (!p1) return 0;
    *p1 = '\0';

    char *p2 = strchr(p1 + 1, ':');
    if (!p2) return 0;
    *p2 = '\0';

    strcpy(field, temp);
    strcpy(op, p1 + 1);
    strcpy(value, p2 + 1);
    return 1;
}

int match_condition(Report *r, const char *field, const char *op, const char *value) {
    if (strcmp(field, "severity") == 0) {
        int val = atoi(value);
        if (strcmp(op, "==") == 0) return r->severity == val;
        if (strcmp(op, "!=") == 0) return r->severity != val;
        if (strcmp(op, ">=") == 0) return r->severity >= val;
        if (strcmp(op, "<=") == 0) return r->severity <= val;
        if (strcmp(op, ">") == 0) return r->severity > val;
        if (strcmp(op, "<") == 0) return r->severity < val;
    } else if (strcmp(field, "timestamp") == 0) {
        time_t val = (time_t)atol(value);
        if (strcmp(op, "==") == 0) return r->timestamp == val;
        if (strcmp(op, "!=") == 0) return r->timestamp != val;
        if (strcmp(op, ">=") == 0) return r->timestamp >= val;
        if (strcmp(op, "<=") == 0) return r->timestamp <= val;
        if (strcmp(op, ">") == 0) return r->timestamp > val;
        if (strcmp(op, "<") == 0) return r->timestamp < val;
    } else if (strcmp(field, "category") == 0) {
        if (strcmp(op, "==") == 0) return strcmp(r->category, value) == 0;
        if (strcmp(op, "!=") == 0) return strcmp(r->category, value) != 0;
    } else if (strcmp(field, "inspector") == 0) {
        if (strcmp(op, "==") == 0) return strcmp(r->inspector, value) == 0;
        if (strcmp(op, "!=") == 0) return strcmp(r->inspector, value) != 0;
    }
    return 0;
}

void check_dangling_link(const char *district) {
    char linkname[256];
    snprintf(linkname, sizeof(linkname), "active_reports-%s", district);
    
    struct stat lst, st;
    if (lstat(linkname, &lst) == 0) {
        if (stat(linkname, &st) == -1) {
            printf("Avertisment: Link-ul simbolic %s este dangling (tinta nu mai exista)!\n", linkname);
        }
    }
}

void init_district(const char *district) {
    struct stat st = {0};
    char filepath[256];

    if (stat(district, &st) == -1) {
        mkdir(district, 0750);
    }

    snprintf(filepath, sizeof(filepath), "%s/reports.dat", district);
    int fd = open(filepath, O_CREAT | O_RDWR, 0664);
    if (fd != -1) close(fd);
    chmod(filepath, 0664);

    snprintf(filepath, sizeof(filepath), "%s/district.cfg", district);
    fd = open(filepath, O_CREAT | O_RDWR, 0640);
    if (fd != -1) close(fd);
    chmod(filepath, 0640);

    snprintf(filepath, sizeof(filepath), "%s/logged_district", district);
    fd = open(filepath, O_CREAT | O_RDWR, 0644);
    if (fd != -1) close(fd);
    chmod(filepath, 0644);

    char targetpath[256];
    snprintf(targetpath, sizeof(targetpath), "%s/reports.dat", district);
    
    char linkname[256];
    snprintf(linkname, sizeof(linkname), "active_reports-%s", district);
    
    if (lstat(linkname, &st) == 0) {
        unlink(linkname); 
    }
    symlink(targetpath, linkname);
}

void add(const char *district, const char *role, const char *user) {
    char filepath[256];
    snprintf(filepath, sizeof(filepath), "%s/reports.dat", district);

    if (!check_permission(filepath, role, S_IWUSR, S_IWGRP)) {
        printf("Eroare: '%s' nu are permisiuni de scriere in %s\n", role, filepath);
        return;
    }

    int fd = open(filepath, O_RDWR | O_APPEND);
    if (fd == -1) return;

    Report r;
    memset(&r, 0, sizeof(Report));

    struct stat st;
    stat(filepath, &st);
    
    if (st.st_size >= (off_t)sizeof(Report)) {
        Report last_r;
        lseek(fd, -sizeof(Report), SEEK_END);
        read(fd, &last_r, sizeof(Report));
        r.id = last_r.id + 1;
    } else {
        r.id = 1;
    }

    strncpy(r.inspector, user, STRING_SIZE - 1);
    r.lat = 45.75;
    r.lon = 21.22;
    strncpy(r.category, "road", STRING_SIZE - 1);
    r.severity = 2;
    r.timestamp = time(NULL);
    strncpy(r.description, "Raport generat in sistem", MAX_SIZE - 1);

    lseek(fd, 0, SEEK_END);
    write(fd, &r, sizeof(Report));
    close(fd);
    printf("Raport %d adaugat in %s.\n", r.id, district);

    char monitor_msg[256] = "monitorul nu a putut fi informat";
    int pid_fd = open(".monitor_pid", O_RDONLY);
    if (pid_fd != -1) {
        char pid_buf[32] = {0};
        read(pid_fd, pid_buf, sizeof(pid_buf) - 1);
        close(pid_fd);
        pid_t monitor_pid = atoi(pid_buf);
        if (monitor_pid > 0 && kill(monitor_pid, SIGUSR1) == 0) {
            strcpy(monitor_msg, "monitor informat cu succes");
        }
    }

    char logpath[256];
    snprintf(logpath, sizeof(logpath), "%s/logged_district", district);
    if (check_permission(logpath, role, S_IWUSR, S_IWGRP)) {
        int log_fd = open(logpath, O_WRONLY | O_APPEND);
        if (log_fd != -1) {
            char log_buffer[512];
            char time_str[64];
            time_t now = time(NULL);
            strcpy(time_str, ctime(&now));
            time_str[strlen(time_str) - 1] = '\0'; 
            
            int len = snprintf(log_buffer, sizeof(log_buffer), "[%s] User: %s, Role: %s | Raport ID %d adaugat | %s\n", time_str, user, role, r.id, monitor_msg);
            write(log_fd, log_buffer, len);
            close(log_fd);
        }
    }
}

void list(const char *district, const char *role) {
    char filepath[256];
    snprintf(filepath, sizeof(filepath), "%s/reports.dat", district);

    if (!check_permission(filepath, role, S_IRUSR, S_IRGRP)) {
        printf("Eroare: '%s' nu are permisiuni de citire in %s\n", role, filepath);
        return;
    }

    struct stat st;
    stat(filepath, &st);

    char perm_str[11];
    mode_to_str(st.st_mode, perm_str);

    printf("Info Fisier: %s | Dimensiune: %ld bytes | Modificat: %s", perm_str, st.st_size, ctime(&st.st_mtime));

    int fd = open(filepath, O_RDONLY);
    if (fd == -1) return;

    Report r;
    printf("--- Rapoarte in %s ---\n", district);
    while (read(fd, &r, sizeof(Report)) == sizeof(Report)) {
        printf("ID: %d | Inspector: %s | Categorie: %s | Severitate: %d\n", r.id, r.inspector, r.category, r.severity);
    }
    close(fd);
}

void view(const char *district, const char *role, int report_id) {
    char filepath[256];
    snprintf(filepath, sizeof(filepath), "%s/reports.dat", district);

    if (!check_permission(filepath, role, S_IRUSR, S_IRGRP)) {
        printf("Eroare: '%s' nu are permisiuni de citire.\n", role);
        return;
    }

    int fd = open(filepath, O_RDONLY);
    if (fd == -1) return;

    Report r;
    int found = 0;
    while (read(fd, &r, sizeof(Report)) == sizeof(Report)) {
        if (r.id == report_id) {
            printf("--- Detalii Raport %d ---\n", r.id);
            printf("Inspector: %s\n", r.inspector);
            printf("Coordonate: GPS(%.4f, %.4f)\n", r.lat, r.lon);
            printf("Categorie: %s\n", r.category);
            printf("Severitate: %d\n", r.severity);
            printf("Data: %s", ctime(&r.timestamp));
            printf("Descriere: %s\n", r.description);
            found = 1;
            break;
        }
    }
    
    if (!found) {
        printf("Raportul cu ID %d nu a fost gasit.\n", report_id);
    }
    close(fd);
}

void update_threshold(const char *district, const char *role, int value) {
    if (strcmp(role, "manager") != 0) {
        printf("Eroare: Doar managerul poate actualiza pragul.\n");
        return;
    }

    char filepath[256];
    snprintf(filepath, sizeof(filepath), "%s/district.cfg", district);

    struct stat st;
    if (stat(filepath, &st) == -1) {
        perror("Eroare la citirea district.cfg");
        return;
    }

    if ((st.st_mode & 0777) != 0640) {
        printf("Permisiunile fisierului district.cfg au fost modificate! Acces refuzat!\n");
        return;
    }

    int fd = open(filepath, O_WRONLY | O_TRUNC);
    if (fd == -1) return;

    char buffer[50];
    int len = snprintf(buffer, sizeof(buffer), "threshold=%d\n", value);
    write(fd, buffer, len);
    close(fd);
    
    printf("Pragul de severitate pentru %s a fost actualizat la %d\n", district, value);
}

void rm(const char *district, const char *role, int report_id) {
    if (strcmp(role, "manager") != 0) {
        printf("Eroare: Doar managerii pot sterge rapoarte\n");
        return;
    }

    char filepath[256];
    snprintf(filepath, sizeof(filepath), "%s/reports.dat", district);
    int fd = open(filepath, O_RDWR);
    if (fd == -1) return;

    Report r;
    off_t pos = 0, found_pos = -1;

    while (read(fd, &r, sizeof(Report)) == sizeof(Report)) {
        if (r.id == report_id) {
            found_pos = pos;
            break;
        }
        pos += sizeof(Report);
    }

    if (found_pos != -1) {
        off_t read_pos = found_pos + sizeof(Report);
        off_t write_pos = found_pos;

        while (1) {
            lseek(fd, read_pos, SEEK_SET);
            if (read(fd, &r, sizeof(Report)) <= 0)
                break;

            lseek(fd, write_pos, SEEK_SET);
            write(fd, &r, sizeof(Report));

            read_pos += sizeof(Report);
            write_pos += sizeof(Report);
        }
        ftruncate(fd, write_pos);
        printf("Raportul %d a fost sters cu succes\n", report_id);
    } else {
        printf("Raportul %d nu a fost gasit\n", report_id);
    }
    close(fd);
}

void filter(const char *district, const char *role, const char *condition) {
    char filepath[256];
    snprintf(filepath, sizeof(filepath), "%s/reports.dat", district);

    if (!check_permission(filepath, role, S_IRUSR, S_IRGRP)) {
        printf("Eroare acces.\n");
        return;
    }

    char field[50], op[10], value[50];
    if (!parse_condition(condition, field, op, value)) {
        printf("Conditie invalida.\n");
        return;
    }

    int fd = open(filepath, O_RDONLY);
    if (fd == -1) return;

    Report r;
    printf("--- Rezultate filtru (%s) ---\n", condition);
    while (read(fd, &r, sizeof(Report)) == sizeof(Report)) {
        if (match_condition(&r, field, op, value)) {
            printf("ID: %d | Inspector: %s | Cat: %s | Sev: %d\n", r.id, r.inspector, r.category, r.severity);
        }
    }
    close(fd);
}

void remove_district(const char *district, const char *role) {
    if (strcmp(role, "manager") != 0) {
        printf("Eroare: doar managerul poate sterge districte.\n");
        return;
    }

    if (!district || strlen(district) == 0 ||
        strcmp(district, "/") == 0 ||
        strcmp(district, ".") == 0 ||
        strcmp(district, "..") == 0 ||
        strchr(district, '/')) {
        printf("Nume district invalid.\n");
        return;
    }

    char linkname[256];
    snprintf(linkname, sizeof(linkname), "active_reports-%s", district);

    pid_t pid = fork();

    if (pid == 0) {
        execlp("rm", "rm", "-rf", district, NULL);
        perror("exec rm failed");
        exit(1);
    } else if (pid > 0) {
        wait(NULL);
        unlink(linkname);
        printf("Districtul %s a fost sters complet.\n", district);
    } else {
        perror("fork failed");
    }
}

int main(int argc, char **argv) {
    char *role = NULL;
    char *user = NULL;
    char *district = NULL;

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--role") == 0 && i + 1 < argc) {
            role = argv[++i];
        } else if (strcmp(argv[i], "--user") == 0 && i + 1 < argc) {
            user = argv[++i];
        }
    }

    if (!role || !user) {
        printf("comanda folosita gresits, utilizarea este: %s --role <role> --user <user> --<comanda> <argumente>\n", argv[0]);
        return 1;
    }

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--add") == 0 && i + 1 < argc) {
            district = argv[++i];
            check_dangling_link(district);
            init_district(district);
            add(district, role, user);
        } else if (strcmp(argv[i], "--list") == 0 && i + 1 < argc) {
            district = argv[++i];
            check_dangling_link(district);
            list(district, role);
        } else if (strcmp(argv[i], "--view") == 0 && i + 2 < argc) {
            district = argv[++i];
            int id = atoi(argv[++i]);
            check_dangling_link(district);
            view(district, role, id);
        } else if (strcmp(argv[i], "--update_threshold") == 0 && i + 2 < argc) {
            district = argv[++i];
            int value = atoi(argv[++i]);
            check_dangling_link(district);
            update_threshold(district, role, value);
        } else if (strcmp(argv[i], "--remove_report") == 0 && i + 2 < argc) { 
            district = argv[++i];
            int id = atoi(argv[++i]);
            check_dangling_link(district);
            rm(district, role, id);
        } else if (strcmp(argv[i], "--filter") == 0 && i + 2 < argc) {
            district = argv[++i];
            char *condition = argv[++i];
            check_dangling_link(district);
            filter(district, role, condition);
        } else if (strcmp(argv[i], "--remove_district") == 0 && i + 1 < argc) {
            district = argv[++i];
            check_dangling_link(district);
            remove_district(district, role);
        }
    }

    return 0;
}