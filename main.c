#include <dirent.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
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
        if (strcmp(op, ">=") == 0) return r->severity >= val;
        if (strcmp(op, "<=") == 0) return r->severity <= val;
    } else if (strcmp(field, "category") == 0) {
        if (strcmp(op, "==") == 0) return strcmp(r->category, value) == 0;
    }
    return 0;
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

    int fd = open(filepath, O_WRONLY | O_APPEND);
    if (fd == -1) return;

    Report r;
    memset(&r, 0, sizeof(Report));

    struct stat st;
    stat(filepath, &st);
    r.id = (st.st_size / sizeof(Report)) + 1;

    strncpy(r.inspector, user, STRING_SIZE - 1);
    r.lat = 45.75;
    r.lon = 21.22;
    strncpy(r.category, "road", STRING_SIZE - 1);
    r.severity = 2;
    r.timestamp = time(NULL);
    strncpy(r.description, "Raport generat in sistem", MAX_SIZE - 1);

    write(fd, &r, sizeof(Report));
    close(fd);
    printf("Raport %d adaugat in %s.\n", r.id, district);
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

int main(int argc, char **argv) {
    char *role = NULL;
    char *user = NULL;
    char *district = NULL;

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--role") == 0 && i + 1 < argc) {
            role = argv[++i];
        } else if (strcmp(argv[i], "--user") == 0 && i + 1 < argc) {
            user = argv[++i];
        } else if (strcmp(argv[i], "--add") == 0 && i + 1 < argc) {
            district = argv[++i];
            init_district(district);
            add(district, role, user);
        } else if (strcmp(argv[i], "--list") == 0 && i + 1 < argc) {
            district = argv[++i];
            list(district, role);
        } else if (strcmp(argv[i], "--view") == 0 && i + 2 < argc) {
            district = argv[++i];
            int id = atoi(argv[++i]);
            view(district, role, id);
        } else if (strcmp(argv[i], "--update_threshold") == 0 && i + 2 < argc) {
            district = argv[++i];
            int value = atoi(argv[++i]);
            update_threshold(district, role, value);
        } else if (strcmp(argv[i], "--remove_report") == 0 && i + 2 < argc) { 
            district = argv[++i];
            int id = atoi(argv[++i]);
            rm(district, role, id);
        } else if (strcmp(argv[i], "--filter") == 0 && i + 2 < argc) {
            district = argv[++i];
            char *condition = argv[++i];
            filter(district, role, condition);
        }
    }

    if (!role || !user) {
        printf("comanda folosita gresits, utilizarea este: %s --role <role> --user <user> --<comanda> <argumente>\n", argv[0]);
    }

    return 0;
}