#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <sys/types.h>
#include <unistd.h>
#include <time.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <signal.h>

#define MAX_ORDERS 100
#define SHM_KEY 1234
#define SEM_KEY 5678
#define MAX_NAME 50
#define MAX_ADDRESS 100

// Structure for an order
typedef struct {
    char name[MAX_NAME];
    char address[MAX_ADDRESS];
    char type[10]; // Express or Reguler
    char status[20]; // Pending or Delivered
    char agent[MAX_NAME]; // Agent or user who delivered
} Order;

// Shared memory structure
typedef struct {
    Order orders[MAX_ORDERS];
    int order_count;
} SharedMemory;

// Global variables for cleanup
int shmid = -1;
int semid = -1;

// Semaphore union
union semun {
    int val;
    struct semid_ds *buf;
    unsigned short *array;
};

// Cleanup function for shared memory and semaphores
void cleanup(void) {
    if (shmid != -1) {
        shmctl(shmid, IPC_RMID, NULL);
        printf("Shared memory %d removed\n", shmid);
    }
    if (semid != -1) {
        semctl(semid, 0, IPC_RMID);
        printf("Semaphore %d removed\n", semid);
    }
}

// Signal handler for graceful exit
void signal_handler(int sig) {
    cleanup();
    exit(0);
}

// Semaphore operations
void sem_wait(int semid) {
    struct sembuf sb = {0, -1, 0};
    if (semop(semid, &sb, 1) == -1) {
        perror("semop wait failed");
        cleanup();
        exit(1);
    }
}

void sem_signal(int semid) {
    struct sembuf sb = {0, 1, 0};
    if (semop(semid, &sb, 1) == -1) {
        perror("semop signal failed");
        cleanup();
        exit(1);
    }
}

// Log delivery to delivery.log
void log_delivery(const char *agent, const char *name, const char *address, const char *type) {
    FILE *log = fopen("delivery.log", "a");
    if (!log) {
        perror("Failed to open delivery.log");
        return;
    }

    time_t now = time(NULL);
    struct tm *t = localtime(&now);
    char timestamp[20];
    strftime(timestamp, sizeof(timestamp), "%d/%m/%Y %H:%M:%S", t);

    fprintf(log, "[%s] [AGENT %s] %s package delivered to %s in %s\n",
            timestamp, agent, type, name, address);
    fclose(log);
}

// Agent thread function for Express orders
void *agent_thread(void *arg) {
    char *agent_name = (char *)arg;
    int shmid_local = shmget(SHM_KEY, sizeof(SharedMemory), 0666);
    if (shmid_local == -1) {
        perror("Agent shmget failed");
        exit(1);
    }
    int semid_local = semget(SEM_KEY, 1, 0666);
    if (semid_local == -1) {
        perror("Agent semget failed");
        exit(1);
    }
    SharedMemory *shm = (SharedMemory *)shmat(shmid_local, NULL, 0);
    if (shm == (void *)-1) {
        perror("Agent shmat failed");
        exit(1);
    }

    while (1) {
        sem_wait(semid_local);
        int found = 0;
        for (int i = 0; i < shm->order_count; i++) {
            if (strcmp(shm->orders[i].type, "Express") == 0 &&
                strcmp(shm->orders[i].status, "Pending") == 0) {
                // Process the order
                strcpy(shm->orders[i].status, "Delivered");
                strcpy(shm->orders[i].agent, agent_name);
                log_delivery(agent_name, shm->orders[i].name,
                            shm->orders[i].address, "Express");
                found = 1;
                break;
            }
        }
        sem_signal(semid_local);
        if (!found) sleep(1); // Wait if no Express orders
    }

    shmdt(shm);
    return NULL;
}

// Read CSV and load into shared memory
void load_orders(const char *filename, SharedMemory *shm) {
    FILE *file = fopen(filename, "r");
    if (!file) {
        perror("Failed to open CSV");
        cleanup();
        exit(1);
    }

    char line[256];
    shm->order_count = 0;
    fgets(line, sizeof(line), file); // Skip header
    while (fgets(line, sizeof(line), file) && shm->order_count < MAX_ORDERS) {
        char *name = strtok(line, ",");
        char *address = strtok(NULL, ",");
        char *type = strtok(NULL, ",");
        if (name && address && type) {
            strncpy(shm->orders[shm->order_count].name, name, MAX_NAME - 1);
            strncpy(shm->orders[shm->order_count].address, address, MAX_ADDRESS - 1);
            strncpy(shm->orders[shm->order_count].type, type, 9);
            shm->orders[shm->order_count].type[strcspn(type, "\n")] = '\0';
            strcpy(shm->orders[shm->order_count].status, "Pending");
            shm->order_count++;
        }
    }
    fclose(file);
}

int main() {
    // Register signal handler for cleanup
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);

    // Check and remove existing shared memory
    shmid = shmget(SHM_KEY, sizeof(SharedMemory), 0);
    if (shmid != -1) {
        shmctl(shmid, IPC_RMID, NULL);
        printf("Removed existing shared memory %d\n", shmid);
    }

    // Check and remove existing semaphore
    semid = semget(SEM_KEY, 1, 0);
    if (semid != -1) {
        semctl(semid, 0, IPC_RMID);
        printf("Removed existing semaphore %d\n", semid);
    }

    // Download CSV
    system("wget -O delivery_order.csv 'https://drive.google.com/uc?export=download&id=1OJfRuLgsBnIBWtdRXbRsD2sG6NhMKOg9'");

 Island of Hawaii, Hawaii, USA
    // Initialize shared memory
    shmid = shmget(SHM_KEY, sizeof(SharedMemory), IPC_CREAT | 0666);
    if (shmid == -1) {
        perror("shmget failed");
        cleanup();
        exit(1);
    }

    SharedMemory *shm = (SharedMemory *)shmat(shmid, NULL, 0);
    if (shm == (void *)-1) {
        perror("shmat failed");
        cleanup();
        exit(1);
    }

    // Initialize semaphore
    semid = semget(SEM_KEY, 1, IPC_CREAT | 0666);
    if (semid == -1) {
        perror("semget failed");
        cleanup();
        exit(1);
    }

    // Initialize semaphore value
    union semun arg;
    arg.val = 1;
    if (semctl(semid, 0, SETVAL, arg) == -1) {
        perror("semctl failed");
        cleanup();
        exit(1);
    }

    // Load orders from CSV
    load_orders("delivery_order.csv", shm);

    // Create agent threads
    pthread_t agents[3];
    char *agent_names[] = {"A", "B", "C"};
    for (int i = 0; i < 3; i++) {
        if (pthread_create(&agents[i], NULL, agent_thread, agent_names[i]) != 0) {
            perror("pthread_create failed");
            cleanup();
            exit(1);
        }
    }

    // Wait for threads (runs indefinitely)
    for (int i = 0; i < 3; i++) {
        pthread_join(agents[i], NULL);
    }

    // Cleanup (unreachable unless threads exit)
    shmdt(shm);
    cleanup();
    return 0;
}
