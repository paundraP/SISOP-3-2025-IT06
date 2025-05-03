#include "shm_common.h"
#include <semaphore.h>
#include <fcntl.h>
#include <pthread.h>


#define RESET "\033[0m"
#define CYAN "\033[1;36m"   
#define GREEN "\033[1;32m"  
#define YELLOW "\033[1;33m" 
#define RED "\033[1;31m"    
#define WHITE "\033[1;37m"  
#define BG_BLACK "\033[40m" 

sem_t *sem;
FILE *log_file;
int notification_active = 0;
pthread_t notification_tid;

void log_action(const char *action) {
    time_t now = time(NULL);
    char *timestamp = ctime(&now);
    timestamp[strlen(timestamp) - 1] = '\0';
    log_file = fopen("hunter.log", "a");
    fprintf(log_file, "[%s] %s\n", timestamp, action);
    fclose(log_file);
}

void *notification_thread(void *arg) {
    struct SystemData *data = (struct SystemData *)arg;
    while (1) {
        sem_wait(sem);
        if (data->num_dungeons > 0) {
            int idx = data->current_notification_index % data->num_dungeons;
            printf("%s\n✨ [NOTIFICATION] New Dungeon: %s (minimum level %d opened!) ✨%s\n", 
                   YELLOW, data->dungeons[idx].name, data->dungeons[idx].min_level, RESET);
            data->current_notification_index++;
            char log_msg[100];
            snprintf(log_msg, 100, "Notified dungeon: %s", data->dungeons[idx].name);
            log_action(log_msg);
        }
        sem_post(sem);
        sleep(3);
    }
    return NULL;
}

void list_dungeons(struct SystemData *data, struct Hunter *hunter) {
    sem_wait(sem);
    printf("%s══════ AVAILABLE DUNGEONS ══════%s\n", CYAN, RESET);
    int count = 0;
    for (int i = 0; i < data->num_dungeons; i++) {
        if (data->dungeons[i].min_level <= hunter->level) {
            printf("%s%d. %s (LEVEL %d+)%s\n", WHITE, count + 1, data->dungeons[i].name, data->dungeons[i].min_level, RESET);
            count++;
        }
    }
    if (count == 0) {
        printf("%sNo dungeons available for your level.%s\n", RED, RESET);
        log_action("Listed dungeons: none available");
    } else {
        log_action("Listed available dungeons");
    }
    sem_post(sem);
    printf("\n%sPress enter to continue...%s\n", WHITE, RESET);
    getchar();
}

void raid_dungeon(struct SystemData *data, struct Hunter *hunter) {
    if (hunter->banned) {
        printf("%s\n[ERROR] You are banned from raiding.%s\n", RED, RESET);
        log_action("Raid attempt: banned");
        return;
    }

    sem_wait(sem);
    printf("%s══════ AVAILABLE DUNGEONS ══════%s\n", CYAN, RESET);
    int count = 0;
    for (int i = 0; i < data->num_dungeons; i++) {
        if (data->dungeons[i].min_level <= hunter->level) {
            printf("%s%d. %s (LEVEL %d+)%s\n", WHITE, count + 1, data->dungeons[i].name, data->dungeons[i].min_level, RESET);
            count++;
        }
    }
    if (count == 0) {
        printf("%sNo dungeons available for your level.%s\n", RED, RESET);
        log_action("Raid attempt: no dungeons");
        sem_post(sem);
        return;
    }

    printf("\n%sChoose Dungeon:%s ", WHITE, RESET);
    int choice;
    scanf("%d", &choice);
    getchar();

    if (choice == 0) {
        printf("%s\n[INFO] Raid cancelled.%s\n", YELLOW, RESET);
        log_action("Raid cancelled");
        sem_post(sem);
        return;
    }

    count = 0;
    int target = -1;
    for (int i = 0; i < data->num_dungeons; i++) {
        if (data->dungeons[i].min_level <= hunter->level) {
            count++;
            if (count == choice) {
                target = i;
                break;
            }
        }
    }

    if (target == -1) {
        printf("%s\n[ERROR] Invalid dungeon.%s\n", RED, RESET);
        log_action("Raid attempt: invalid dungeon");
        sem_post(sem);
        return;
    }

    struct Dungeon *d = &data->dungeons[target];
    int dungeon_shmid = shmget(d->shm_key, sizeof(struct Dungeon), 0666);
    if (dungeon_shmid == -1) {
        printf("%s\n[ERROR] Failed to access dungeon shared memory! (SHM Key: %d)%s\n", RED, d->shm_key, RESET);
        log_action("Raid attempt: failed to access shared memory");
        sem_post(sem);
        return;
    }

    struct Dungeon *d_shared = (struct Dungeon *)shmat(dungeon_shmid, NULL, 0);
    if (d_shared == (void *)-1) {
        printf("%s\n[ERROR] Failed to attach dungeon shared memory! (SHM Key: %d)%s\n", RED, d->shm_key, RESET);
        log_action("Raid attempt: failed to attach shared memory");
        shmctl(dungeon_shmid, IPC_RMID, NULL);
        sem_post(sem);
        return;
    }

    hunter->exp += d_shared->exp;
    hunter->atk += d_shared->atk;
    hunter->hp += d_shared->hp;
    hunter->def += d_shared->def;
    char log_msg[150];
    snprintf(log_msg, 150, "Raided %s: gained %d EXP, %d ATK, %d HP, %d DEF", d_shared->name, d_shared->exp, d_shared->atk, d_shared->hp, d_shared->def);
    printf("%s\n⚔️ RAID SUCCESS! GAINED: ⚔️%s\n", YELLOW, RESET);
    printf("%sATK: %d\nHP: %d\nDEF: %d\nEXP: %d%s\n", WHITE, d_shared->atk, d_shared->hp, d_shared->def, d_shared->exp, RESET);
    log_action(log_msg);

    shmdt(d_shared);
    shmctl(dungeon_shmid, IPC_RMID, NULL);

    for (int i = target; i < data->num_dungeons - 1; i++) {
        data->dungeons[i] = data->dungeons[i + 1];
    }
    data->num_dungeons--;
    sem_post(sem);

    sem_wait(sem);
    while (hunter->exp >= 500) {
        hunter->exp -= 500;
        hunter->level++;
        char lvl_msg[100];
        snprintf(lvl_msg, 100, "%s leveled up to %d!", hunter->username, hunter->level);
        printf("%s\n✨ [SUCCESS] %s ✨%s\n", YELLOW, lvl_msg, RESET);
        log_action(lvl_msg);
    }
    sem_post(sem);
}

void battle_hunter(struct SystemData *data, struct Hunter *hunter, int hunter_index) {
    if (hunter->banned) {
        printf("%s\n[ERROR] You are banned from battling.%s\n", RED, RESET);
        log_action("Battle attempt: banned");
        return;
    }

    sem_wait(sem);
    printf("%s══════ PVP LIST ══════%s\n", CYAN, RESET);
    int count = 0;
    for (int i = 0; i < data->num_hunters; i++) {
        if (i != hunter_index && !data->hunters[i].banned) {
            int opponent_total = data->hunters[i].atk + data->hunters[i].hp + data->hunters[i].def;
            printf("%sTarget %d: %s (Power: %d)%s\n", WHITE, count + 1, data->hunters[i].username, opponent_total, RESET);
            count++;
        }
    }

    if (count == 0) {
        printf("%sNo available opponents to battle.%s\n", RED, RESET);
        log_action("Battle attempt: no opponents");
        sem_post(sem);
        return;
    }

    printf("\n%sYour power: %d%s\n", WHITE, hunter->atk + hunter->hp + hunter->def, RESET);
    printf("%sEnter target number to battle (0 to cancel):%s ", WHITE, RESET);
    int choice;
    scanf("%d", &choice);
    getchar();

    if (choice == 0) {
        printf("%s\n[INFO] Battle cancelled.%s\n", YELLOW, RESET);
        log_action("Battle cancelled");
        sem_post(sem);
        return;
    }

    count = 0;
    struct Hunter *opponent = NULL;
    int opponent_index = -1;
    for (int i = 0; i < data->num_hunters; i++) {
        if (i != hunter_index && !data->hunters[i].banned) {
            count++;
            if (count == choice) {
                opponent = &data->hunters[i];
                opponent_index = i;
                break;
            }
        }
    }

    if (opponent == NULL) {
        printf("%s\n[ERROR] Invalid target.%s\n", RED, RESET);
        log_action("Battle attempt: invalid target");
        sem_post(sem);
        return;
    }

    int hunter_total = hunter->atk + hunter->hp + hunter->def;
    int opponent_total = opponent->atk + opponent->hp + opponent->def;

    printf("\n%sTarget chooses to battle %s%s\n", WHITE, opponent->username, RESET);
    printf("%sYour enemy's power: %d%s\n", WHITE, opponent_total, RESET);

    char log_msg[150];
    if (hunter_total > opponent_total) {
        hunter->atk += opponent->atk;
        hunter->hp += opponent->hp;
        hunter->def += opponent->def;
        snprintf(log_msg, 150, "%s won! Acquired %s's stats (ShmId: %d)", hunter->username, opponent->username, opponent->shm_key);
        printf("%s%s won! You acquired %s's stats (ShmId: %d)%s\n", YELLOW, hunter->username, opponent->username, opponent->shm_key, RESET);
        log_action(log_msg);
        for (int i = opponent_index; i < data->num_hunters - 1; i++) {
            data->hunters[i] = data->hunters[i + 1];
        }
        data->num_hunters--;
    } else {
        opponent->atk += hunter->atk;
        opponent->hp += hunter->hp;
        opponent->def += hunter->def;
        snprintf(log_msg, 150, "%s won! Acquired %s's stats (ShmId: %d)", opponent->username, hunter->username, hunter->shm_key);
        printf("%s%s won! Acquired %s's stats (ShmId: %d)%s\n", YELLOW, opponent->username, hunter->username, hunter->shm_key, RESET);
        log_action(log_msg);
        for (int i = hunter_index; i < data->num_hunters - 1; i++) {
            data->hunters[i] = data->hunters[i + 1];
        }
        data->num_hunters--;
        sem_post(sem);
        exit(0);
    }
    sem_post(sem);
}

int main() {
    srand(time(NULL));
    log_file = fopen("hunter.log", "a");
    if (!log_file) {
        perror("fopen log");
        exit(1);
    }

    sem = sem_open("/hunter_sem", 0);
    if (sem == SEM_FAILED) {
        perror("sem_open");
        fprintf(stderr, "%s[ERROR] Make sure system.c is running first!%s\n", RED, RESET);
        fclose(log_file);
        exit(1);
    }

    key_t key = get_system_key();
    int shmid = shmget(key, sizeof(struct SystemData), 0666);
    if (shmid == -1) {
        perror("shmget");
        sem_close(sem);
        fclose(log_file);
        exit(1);
    }

    struct SystemData *data = (struct SystemData *)shmat(shmid, NULL, 0);
    if (data == (void *)-1) {
        perror("shmat");
        sem_close(sem);
        fclose(log_file);
        exit(1);
    }

    log_action("Hunter client started");

    printf("%s\n🛡️══════ HUNTER OF SHADOWS ══════🛡️%s\n", CYAN, RESET);
    printf("%sWelcome, Brave Hunter!%s\n", YELLOW, RESET);
    printf("%sEmbark on your journey to conquer dungeons...%s\n\n", WHITE, RESET);

    while (1) {
        printf("%s╔═══════════════════════╗%s\n", CYAN, RESET);
        printf("%s║     HUNTER MENU      ║%s\n", CYAN, RESET);
        printf("%s╠═══════════════════════╣%s\n", CYAN, RESET);
        printf("%s║ %s1. Register%s          ║%s\n", CYAN, GREEN, CYAN, RESET);
        printf("%s║ %s2. Login%s             ║%s\n", CYAN, GREEN, CYAN, RESET);
        printf("%s║ %s3. Exit%s              ║%s\n", CYAN, GREEN, CYAN, RESET);
        printf("%s╚═══════════════════════╝%s\n", CYAN, RESET);
        printf("%sChoice:%s ", WHITE, RESET);
        int choice;
        scanf("%d", &choice);
        getchar();

        if (choice == 3) {
            if (notification_active) pthread_cancel(notification_tid);
            shmdt(data);
            sem_close(sem);
            log_action("Hunter client exited");
            fclose(log_file);
            printf("%s\n🛡️ [SUCCESS] Hunter client exited. 🛡️%s\n", YELLOW, RESET);
            return 0;
        }

        if (choice == 1) {
            char username[50];
            printf("\n%sUsername:%s ", WHITE, RESET);
            fgets(username, 50, stdin);
            username[strcspn(username, "\n")] = 0;

            sem_wait(sem);
            for (int i = 0; i < data->num_hunters; i++) {
                if (strcmp(data->hunters[i].username, username) == 0) {
                    printf("%s\n[ERROR] Username taken.%s\n", RED, RESET);
                    log_action("Registration failed: username taken");
                    sem_post(sem);
                    continue;
                }
            }

            if (data->num_hunters < MAX_HUNTERS) {
                struct Hunter *h = &data->hunters[data->num_hunters];
                strcpy(h->username, username);
                h->level = 1;
                h->exp = 0;
                h->atk = 10;
                h->hp = 100;
                h->def = 5;
                h->banned = 0;
                h->shm_key = ftok("/tmp", 'H' + data->num_hunters);
                data->num_hunters++;
                char log_msg[100];
                snprintf(log_msg, 100, "Registered hunter: %s", username);
                printf("%s\n✨ [SUCCESS] Registration successful! ✨%s\n", YELLOW, RESET);
                log_action(log_msg);
            } else {
                printf("%s\n[ERROR] Hunter limit reached.%s\n", RED, RESET);
                log_action("Registration failed: hunter limit reached");
            }
            sem_post(sem);
        } else if (choice == 2) {
            char username[50];
            printf("\n%sUsername:%s ", WHITE, RESET);
            fgets(username, 50, stdin);
            username[strcspn(username, "\n")] = 0;

            sem_wait(sem);
            struct Hunter *hunter = NULL;
            int hunter_index = -1;
            for (int i = 0; i < data->num_hunters; i++) {
                if (strcmp(data->hunters[i].username, username) == 0) {
                    hunter = &data->hunters[i];
                    hunter_index = i;
                    break;
                }
            }
            sem_post(sem);

            if (hunter == NULL) {
                printf("%s\n[ERROR] Hunter not found.%s\n", RED, RESET);
                log_action("Login failed: hunter not found");
                continue;
            }

            char log_msg[100];
            snprintf(log_msg, 100, "Logged in as %s", username);
            printf("%s\n✨ [SUCCESS] Logged in as %s! ✨%s\n", YELLOW, username, RESET);
            log_action(log_msg);

            while (1) {
                printf("%s\n--- HUNTER SYSTEM ---%s\n", CYAN, RESET);
                printf("%s[%s's MENU --- minimum level %d opened!]%s\n", YELLOW, hunter->username, hunter->level, RESET);
                printf("%s╔═══════════════════════╗%s\n", CYAN, RESET);
                printf("%s║     HUNTER SYSTEM    ║%s\n", CYAN, RESET);
                printf("%s╠═══════════════════════╣%s\n", CYAN, RESET);
                printf("%s║ %s1. Dungeon List%s      ║%s\n", CYAN, GREEN, CYAN, RESET);
                printf("%s║ %s2. Dungeon Raid%s      ║%s\n", CYAN, GREEN, CYAN, RESET);
                printf("%s║ %s3. Hunters Battle%s    ║%s\n", CYAN, GREEN, CYAN, RESET);
                printf("%s║ %s4. Notification%s      ║%s\n", CYAN, GREEN, CYAN, RESET);
                printf("%s║ %s5. Exit%s              ║%s\n", CYAN, GREEN, CYAN, RESET);
                printf("%s╚═══════════════════════╝%s\n", CYAN, RESET);
                printf("%sCHOICE:%s ", WHITE, RESET);
                scanf("%d", &choice);
                getchar();

                switch (choice) {
                    case 1: list_dungeons(data, hunter); break;
                    case 2: raid_dungeon(data, hunter); break;
                    case 3: battle_hunter(data, hunter, hunter_index); break;
                    case 4:
                        notification_active = !notification_active;
                        if (notification_active) {
                            pthread_create(&notification_tid, NULL, notification_thread, data);
                            printf("%s\n✨ [SUCCESS] Notifications enabled. ✨%s\n", YELLOW, RESET);
                            log_action("Notifications enabled");
                        } else {
                            pthread_cancel(notification_tid);
                            printf("%s\n✨ [SUCCESS] Notifications disabled. ✨%s\n", YELLOW, RESET);
                            log_action("Notifications disabled");
                        }
                        break;
                    case 5:
                        if (notification_active) pthread_cancel(notification_tid);
                        printf("%s\n✨ [SUCCESS] Logged out. ✨%s\n", YELLOW, RESET);
                        log_action("Logged out");
                        break;
                    default:
                        printf("%s\n[ERROR] Invalid option.%s\n", RED, RESET);
                        log_action("Invalid hunter menu option");
                }
                if (choice == 5) break;

                sem_wait(sem);
                while (hunter->exp >= 500) {
                    hunter->exp -= 500;
                    hunter->level++;
                    char lvl_msg[100];
                    snprintf(lvl_msg, 100, "%s leveled up to %d!", hunter->username, hunter->level);
                    printf("%s\n✨ [SUCCESS] %s ✨%s\n", YELLOW, lvl_msg, RESET);
                    log_action(lvl_msg);
                }
                sem_post(sem);
            }
        } else {
            printf("%s\n[ERROR] Invalid option.%s\n", RED, RESET);
            log_action("Invalid main menu option");
        }
    }
}
