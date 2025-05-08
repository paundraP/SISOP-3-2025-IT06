# SISOP-3-2025-IT06

Repository ini berisi hasil pengerjaan Praktikum Sistem Operasi 2025 Modul 3

| Nama                     | Nrp        |
| ------------------------ | ---------- |
| Paundra Pujo Darmawan    | 5027241008 |
| Putri Joselina Silitonga | 5027241116 |


##SOAL 2 (Putri Joselina Silitonga)

#Delivery_agent

1. download_file — Mengunduh File CSV

```
void download_file() {
    char command[512];
    sprintf(command, "wget -O delivery_order.csv \"https://drive.google.com/uc?export=download&id=1OJfRuLgsBnIBWtdRXbRsD2sG6NhMKOg9\"");
    system(command);
}
```

Fungsi ini mengunduh file delivery_order.csv dari Google Drive menggunakan wget.

2. read_csv — Membaca dan Memasukkan Data ke Shared Memory
```
void read_csv() {
    FILE* fp = fopen("delivery_order.csv", "r");
    if (!fp) {
        perror("Failed to open CSV");
        exit(1);
    }

    char line[200];
    fgets(line, sizeof(line), fp); // Skip header
    while (fgets(line, sizeof(line), fp) && order_count < SHM_SIZE / sizeof(Order)) {
        Order order;
        sscanf(line, "%[^,],%[^,],%s", order.name, order.address, order.type);
        order.delivered = 0;
        shared_orders[order_count++] = order;
    }
    fclose(fp);
}
```
Fungsi ini membaca file delivery_order.csv, mem-parse baris per baris, dan menyimpan data ke array shared memory.

3. log_delivery — Mencatat Log Pengiriman
```
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
```
Fungsi ini membuat entri log setiap kali sebuah paket Express berhasil dikirim.

4. all_express_delivered — Mengecek Status Semua Order Express
```
int all_express_delivered() {
    for (int i = 0; i < order_count; i++) {
        if (strcmp(shared_orders[i].type, "Express") == 0 && shared_orders[i].delivered == 0) {
            return 0;
        }
    }
    return 1;
}
```
Fungsi ini mengembalikan 1 jika semua pesanan bertipe Express telah dikirim, atau 0 jika masih ada yang tersisa.

5. agent_thread — Thread Agen Pengiriman
```
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
```
Thread ini bertugas mengirimkan paket secara paralel. Setiap agen mengirimkan pesanan Express secara bergiliran.

6. main 
```
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
```
Fungsi main mengatur shared memory, memanggil fungsi-fungsi utama, dan menjalankan tiga thread agen pengiriman.


##dispacther.c 
1. Read Share Memory
```
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
```
Fungsinya untuk membaca shared memory yang berisi daftar pesanan dan menghitung jumlah pesanan (order_count).


2. Log Delivery 
```
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
``` 
Fungsinya untuk mencatat pengiriman paket ke file delivery.log dengan timestamp, nama agen, tipe paket, nama penerima, dan alamat.

3. main
```
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
```
Fungsinya untuk mengatur alur utama program, membaca shared memory, dan memproses perintah command line (-deliver, -status, atau -list).

## SOAL 3 (Paundra Pujo Darmawan)

To compile the dungeon

```bash
gcc shop.c dungeon.c -o dungeon -I/opt/homebrew/include -L/opt/homebrew/lib -lcjson
```

To compile the player

```bash
gcc player.c -o player -I/opt/homebrew/include -L/opt/homebrew/lib -lcjson
```
