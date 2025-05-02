#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <pthread.h>
#include <time.h>

#define SHM_SIZE 8192
#define SHM_NAME "/delivery_orders"

typedef struct {
    char name[50];
    char address[100];
    char type[10];
    int delivered;
} Order;

Order* shared_orders;
int order_count = 0;

void download_file() {
    char command[512];
    sprintf(command, "wget -O delivery_order.csv \"https://drive.google.com/uc?export=download&id=1OJfRuLgsBnIBWtdRXbRsD2sG6NhMKOg9\"");
    system(command);
}

void read_csv() {
    FILE* fp = fopen("delivery_order.csv", "r");
    if (!fp) {
        perror("Failed to open CSV");
        exit(1);
    }

    char line[200];
    fgets(line, sizeof(line), fp); 
    while (fgets(line, sizeof(line), fp) && order_count < SHM_SIZE / sizeof(Order)) {
        Order order;
        sscanf(line, "%[^,],%[^,],%s", order.name, order.address, order.type);
        order.delivered = 0;
        shared_orders[order_count++] = order;
    }
    fclose(fp);
}

void log_delivery(const char* agent, const char* name, const char* address) {
    FILE* log_file = fopen("delivery.log", "a");
    if (log_file) {
        time_t t = time(NULL);
        struct tm* tm = localtime(&t);
        fprintf(log_file, "[%02d/%02d/%04d %02d:%02d:%02d] [%s] Express package delivered to %s in %s\n",
                tm->tm_mday, tm->tm_mon + 1, tm->tm_year + 1900,
                tm->tm_hour, tm->tm_min, tm->tm_sec, agent, name, address);
        fclose(log_file);
    }
}

int all_express_delivered() {
    for (int i = 0; i < order_count; i++) {
        if (strcmp(shared_orders[i].type, "Express") == 0 && shared_orders[i].delivered == 0) {
            return 0;  
        }
    }
    return 1;  
}

void* agent_thread(void* arg) {
    const char* agent_id = (const char*)arg;
    while (1) {
        int all_delivered = 1;
        for (int i = 0; i < order_count; i++) {
            if (shared_orders[i].delivered == 0 && strcmp(shared_orders[i].type, "Express") == 0) {
                shared_orders[i].delivered = 1;
                printf("%s Express package delivered to %s in %s\n", agent_id, shared_orders[i].name, shared_orders[i].address);
                log_delivery(agent_id, shared_orders[i].name, shared_orders[i].address);
                sleep(1); 
                all_delivered = 0;  
            }
        }
        
        if (all_delivered) {
            printf("All express orders have been delivered. Exiting...\n");
            exit(0);  
        }

        sleep(1); 
    }
    return NULL;
}

int main() {
    int shm_fd = shm_open(SHM_NAME, O_CREAT | O_RDWR, 0666);
    ftruncate(shm_fd, SHM_SIZE);
    shared_orders = (Order*) mmap(0, SHM_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);

    download_file();
    read_csv();

    pthread_t threads[3];
    const char* agents[] = {"Agent A", "Agent B", "Agent C"};

    for (int i = 0; i < 3; i++) {
        pthread_create(&threads[i], NULL, agent_thread, (void*)agents[i]);
    }

    for (int i = 0; i < 3; i++) {
        pthread_join(threads[i], NULL);
    }

    munmap(shared_orders, SHM_SIZE);
    shm_unlink(SHM_NAME);
    return 0;
}
