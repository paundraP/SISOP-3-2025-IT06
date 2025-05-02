#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
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

void read_shared_memory() {
    int shm_fd = shm_open(SHM_NAME, O_RDWR, 0666);
    if (shm_fd == -1) {
        perror("shm_open");
        exit(1);
    }

    shared_orders = (Order*) mmap(0, SHM_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
    if (shared_orders == MAP_FAILED) {
        perror("mmap");
        exit(1);
    }

    
    order_count = 0;
    while (shared_orders[order_count].name[0] != '\0' && order_count < SHM_SIZE / sizeof(Order)) {
        order_count++;
    }
}

void log_delivery(const char* agent, const char* name, const char* address, const char* type) {
    FILE* log_file = fopen("delivery.log", "a");
    if (log_file) {
        time_t t = time(NULL);
        struct tm* tm = localtime(&t);
        fprintf(log_file, "[%02d/%02d/%04d %02d:%02d:%02d] [%s] %s package delivered to %s in %s\n",
                tm->tm_mday, tm->tm_mon + 1, tm->tm_year + 1900,
                tm->tm_hour, tm->tm_min, tm->tm_sec, agent, type, name, address);
        fclose(log_file);
    }
}

int main(int argc, char* argv[]) {
    read_shared_memory();

    if (argc < 2) {
        printf("Usage: %s -deliver [Name] or -status [Name] or -list\n", argv[0]);
        return 1;
    }

    if (strcmp(argv[1], "-deliver") == 0 && argc == 3) {
        char* recipient = argv[2];
        char agent[50];
        snprintf(agent, sizeof(agent), "Agent %s", getenv("USER") ? getenv("USER") : "Unknown");
        for (int i = 0; i < order_count; i++) {
            if (strcmp(shared_orders[i].name, recipient) == 0 &&
                strcmp(shared_orders[i].type, "Reguler") == 0 &&
                !shared_orders[i].delivered) {

                shared_orders[i].delivered = 1;
                printf("Reguler package delivered to %s by %s\n", recipient, agent);
                log_delivery(agent, recipient, shared_orders[i].address, "Reguler");
                return 0;
            }
        }
        printf("No Reguler order found for %s\n", recipient);
    }
    else if (strcmp(argv[1], "-status") == 0 && argc == 3) {
        char* name = argv[2];
        for (int i = 0; i < order_count; i++) {
            if (strcmp(shared_orders[i].name, name) == 0) {
                if (shared_orders[i].delivered) {
                    printf("Status for %s: Delivered by Agent %s\n", name,
                        shared_orders[i].type[0] == 'E' ? "C" : (getenv("USER") ? getenv("USER") : "Unknown"));
                } else {
                    printf("Status for %s: Pending\n", name);
                }
                return 0;
            }
        }
        printf("No order found for %s\n", name);
    }
    else if (strcmp(argv[1], "-list") == 0) {
        if (order_count == 0) {
            printf("No orders found.\n");
        } else {
            for (int i = 0; i < order_count; i++) {
                printf("%s - %s\n", shared_orders[i].name, shared_orders[i].delivered ? "Delivered" : "Pending");
            }
        }
    }
    else {
        printf("Invalid command. Use -deliver [Name], -status [Name], or -list\n");
    }

    munmap(shared_orders, SHM_SIZE);
    return 0;
}
