#include "shm_common.h"
#include <semaphore.h>
#include <fcntl.h>
#include <signal.h>

#define RESET "\033[0m"
#define CYAN "\033[1;36m"   
#define GREEN "\033[1;32m"  
#define YELLOW "\033[1;33m" 
#define RED "\033[1;31m"    
#define WHITE "\033[1;37m"  
#define BG_BLACK "\033[40m" 

sem_t *sem;
FILE *log_file;
int shmid;

void log_action(const char *action) {
    time_t now = time(NULL);
    char *timestamp = ctime(&now);
    timestamp[strlen(timestamp) - 1] = '\0';
    log_file = fopen("system.log", "a");
    fprintf(log_file, "[%s] %s\n", timestamp, action);
    fclose(log_file);
}

void display_hunter_info(struct SystemData *data) {
    sem_wait(sem);
    printf("%s══════ HUNTER LIST ══════%s\n", CYAN, RESET);
    for (int i = 0; i < data->num_hunters; i++) {
        printf("%sHunter %d:%s\n", WHITE, i + 1, RESET);
        printf("  Name: %s\n", data->hunters[i].username);
        printf("  Level: %d\n", data->hunters[i].level);
        printf("  EXP: %d\n", data->hunters[i].exp);
        printf("  ATK: %d\n", data->hunters[i].atk);
        printf("  HP: %d\n", data->hunters[i].hp);
        printf("  DEF: %d\n", data->hunters[i].def);
        printf("  Banned: %s\n", data->hunters[i].banned ? "Yes" : "No");
    }
    if (data->num_hunters == 0) {
        printf("%sNo hunters registered.%s\n", RED, RESET);
    }
    log_action("Displayed hunter info");
    sem_post(sem);
}

void display_dungeon_info(struct SystemData *data) {
    sem_wait(sem);
    printf("%s══════ DUNGEON LIST ══════%s\n", CYAN, RESET);
    for (int i = 0; i < data->num_dungeons; i++) {
        printf("%sDungeon %d:%s\n", WHITE, i + 1, RESET);
        printf("  Name: %s\n", data->dungeons[i].name);
        printf("  Minimum Level: %d\n", data->dungeons[i].min_level);
        printf("  Reward - EXP: %d, ATK: %d, HP: %d, DEF: %d\n", 
               data->dungeons[i].exp, data->dungeons[i].atk, data->dungeons[i].hp, data->dungeons[i].def);
        printf("  Key: %d\n", data->dungeons[i].shm_key);
    }
    if (data->num_dungeons == 0) {
        printf("%sNo dungeons available.%s\n", RED, RESET);
    }
    log_action("Displayed dungeon info");
    sem_post(sem);
}

void generate_dungeon(struct SystemData *data) {
    sem_wait(sem);
    if (data->num_dungeons >= MAX_DUNGEONS) {
        printf("%s[ERROR] Maximum dungeon limit reached!%s\n", RED, RESET);
        log_action("Generate dungeon failed: limit reached");
        sem_post(sem);
        return;
    }

    struct Dungeon d;
    char *locations[] = {"Jeju", "Tokyo", "Seoul", "Busan", "Osaka", "Kyoto"};
    char *ranks[] = {"S-Rank", "A-Rank", "B-Rank", "C-Rank", "D-Rank"};
    snprintf(d.name, 50, "%s %s Dungeon", locations[rand() % 6], ranks[rand() % 5]);
    d.min_level = (rand() % 5) + 1;
    d.exp = (rand() % 151) + 150;
    d.atk = (rand() % 51) + 100;
    d.hp = (rand() % 51) + 50;
    d.def = (rand() % 26) + 25;
    d.shm_key = 123456789 + data->num_dungeons;

    int dungeon_shmid = shmget(d.shm_key, sizeof(struct Dungeon), IPC_CREAT | 0666);
    if (dungeon_shmid == -1) {
        printf("%s[ERROR] Failed to create dungeon shared memory!%s\n", RED, RESET);
        log_action("Generate dungeon failed: shmget error");
        sem_post(sem);
        return;
    }

    struct Dungeon *d_shared = (struct Dungeon *)shmat(dungeon_shmid, NULL, 0);
    if (d_shared == (void *)-1) {
        printf("%s[ERROR] Failed to attach dungeon shared memory!%s\n", RED, RESET);
        log_action("Generate dungeon failed: shmat error");
        shmctl(dungeon_shmid, IPC_RMID, NULL);
        sem_post(sem);
        return;
    }

    *d_shared = d;
    data->dungeons[data->num_dungeons] = d;
    data->num_dungeons++;
    data->current_notification_index = 0;

    printf("%s✨ [SUCCESS] Dungeon generated! ✨%s\n", YELLOW, RESET);
    printf("  Name: %s\n", d.name);
    printf("  Minimum Level: %d\n", d.min_level);
    printf("  Reward - EXP: %d, ATK: %d, HP: %d, DEF: %d\n", d.exp, d.atk, d.hp, d.def);
    printf("  Key: %d\n", d.shm_key);

    char log_msg[100];
    snprintf(log_msg, 100, "Generated dungeon: %s", d.name);
    log_action(log_msg);
    shmdt(d_shared);
    sem_post(sem);
}

void ban_hunter(struct SystemData *data) {
    printf("\nEnter username to ban/unban: ");
    char username[50];
    fgets(username, 50, stdin);
    username[strcspn(username, "\n")] = 0;

    sem_wait(sem);
    for (int i = 0; i < data->num_hunters; i++) {
        if (strcmp(data->hunters[i].username, username) == 0) {
            data->hunters[i].banned = !data->hunters[i].banned;
            printf("%s[SUCCESS] %s is now %sbanned%s\n", YELLOW, username, 
                   data->hunters[i].banned ? "" : "un", RESET);
            char log_msg[100];
            snprintf(log_msg, 100, "%s %s", username, data->hunters[i].banned ? "banned" : "unbanned");
            log_action(log_msg);
            sem_post(sem);
            return;
        }
    }
    printf("%s[ERROR] Hunter not found.%s\n", RED, RESET);
    log_action("Ban attempt: hunter not found");
    sem_post(sem);
}

void reset_hunter(struct SystemData *data) {
    printf("\nEnter username to reset: ");
    char username[50];
    fgets(username, 50, stdin);
    username[strcspn(username, "\n")] = 0;

    sem_wait(sem);
    for (int i = 0; i < data->num_hunters; i++) {
        if (strcmp(data->hunters[i].username, username) == 0) {
            data->hunters[i].level = 1;
            data->hunters[i].exp = 0;
            data->hunters[i].atk = 10;
            data->hunters[i].hp = 100;
            data->hunters[i].def = 5;
            printf("%s[SUCCESS] Reset stats for %s%s\n", YELLOW, username, RESET);
            char log_msg[100];
            snprintf(log_msg, 100, "Reset stats for %s", username);
            log_action(log_msg);
            sem_post(sem);
            return;
        }
    }
    printf("%s[ERROR] Hunter not found.%s\n", RED, RESET);
    log_action("Reset attempt: hunter not found");
    sem_post(sem);
}

void cleanup(int sig) {
    sem_close(sem);
    sem_unlink("/hunter_sem");
    for (int i = 0; i < MAX_DUNGEONS; i++) {
        int dungeon_shmid = shmget(123456789 + i, sizeof(struct Dungeon), 0666);
        if (dungeon_shmid != -1) shmctl(dungeon_shmid, IPC_RMID, NULL);
    }
    shmctl(shmid, IPC_RMID, NULL);
    log_action("System terminated");
    fclose(log_file);
    printf("%s⚔️ [SUCCESS] System terminated. ⚔️%s\n", YELLOW, RESET);
    exit(0);
}

int main() {
    srand(time(NULL));
    log_file = fopen("system.log", "a");
    if (!log_file) {
        perror("fopen log");
        exit(1);
    }

    sem_unlink("/hunter_sem");
    sem = sem_open("/hunter_sem", O_CREAT, 0644, 1);
    if (sem == SEM_FAILED) {
        perror("sem_open");
        fclose(log_file);
        exit(1);
    }

    key_t key = get_system_key();
    shmid = shmget(key, sizeof(struct SystemData), IPC_CREAT | 0666);
    if (shmid == -1) {
        perror("shmget");
        sem_close(sem);
        sem_unlink("/hunter_sem");
        fclose(log_file);
        exit(1);
    }

    struct SystemData *data = (struct SystemData *)shmat(shmid, NULL, 0);
    if (data == (void *)-1) {
        perror("shmat");
        shmctl(shmid, IPC_RMID, NULL);
        sem_close(sem);
        sem_unlink("/hunter_sem");
        fclose(log_file);
        exit(1);
    }

    data->num_hunters = 0;
    data->num_dungeons = 0;
    data->current_notification_index = 0;
    log_action("System started");

    signal(SIGINT, cleanup);
    signal(SIGTERM, cleanup);

    printf("%s\n⚔️══════ THE LOST DUNGEON SYSTEM ══════⚔️%s\n", CYAN, RESET);
    printf("%sWelcome, Administrator of Shadows!%s\n", YELLOW, RESET);
    printf("%sManage dungeons and hunters with great power...%s\n\n", WHITE, RESET);

    while (1) {
        printf("%s╔═══════════════════════╗%s\n", CYAN, RESET);
        printf("%s║     SYSTEM MENU      ║%s\n", CYAN, RESET);
        printf("%s╠═══════════════════════╣%s\n", CYAN, RESET);
        printf("%s║ %s1. HUNTER INFO%s       ║%s\n", CYAN, GREEN, CYAN, RESET);
        printf("%s║ %s2. DUNGEON INFO%s      ║%s\n", CYAN, GREEN, CYAN, RESET);
        printf("%s║ %s3. GENERATE DUNGEON%s  ║%s\n", CYAN, GREEN, CYAN, RESET);
        printf("%s║ %s4. BAN HUNTER%s        ║%s\n", CYAN, GREEN, CYAN, RESET);
        printf("%s║ %s5. RESET HUNTER%s      ║%s\n", CYAN, GREEN, CYAN, RESET);
        printf("%s║ %s6. EXIT%s              ║%s\n", CYAN, GREEN, CYAN, RESET);
        printf("%s╚═══════════════════════╝%s\n", CYAN, RESET);
        printf("%sCHOICE:%s ", WHITE, RESET);
        int choice;
        scanf("%d", &choice);
        getchar();

        switch (choice) {
            case 1: display_hunter_info(data); break;
            case 2: display_dungeon_info(data); break;
            case 3: generate_dungeon(data); break;
            case 4: ban_hunter(data); break;
            case 5: reset_hunter(data); break;
            case 6: cleanup(0); break;
            default:
                printf("%s[ERROR] Invalid option.%s\n", RED, RESET);
                log_action("Invalid menu option");
        }
    }
}
