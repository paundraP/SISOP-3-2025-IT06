# SISOP-3-2025-IT06

Repository ini berisi hasil pengerjaan Praktikum Sistem Operasi 2025 Modul 3

| Nama                     | Nrp        |
| ------------------------ | ---------- |
| Paundra Pujo Darmawan    | 5027241008 |
| Putri Joselina Silitonga | 5027241116 |

### SOAL 1 (Paundra Pujo Darmawan)

Pada soal ini, kita diminta untuk membuat

# SOAL 2 (Putri Joselina Silitonga)

# Delivery_agent

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

# dispatcher.c

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

### SOAL 3 (Paundra Pujo Darmawan)

To compile the dungeon

```bash
gcc shop.c dungeon.c -o dungeon -I/opt/homebrew/include -L/opt/homebrew/lib -lcjson
```

To compile the player

```bash
gcc player.c -o player -I/opt/homebrew/include -L/opt/homebrew/lib -lcjson
```


## SOAL 4 (Putri Joselina Silitonga) 
# system.c 
1. Log action 
```
void log_action(const char *action) {
    time_t now = time(NULL);
    char *timestamp = ctime(&now);
    timestamp[strlen(timestamp) - 1] = '\0';
    log_file = fopen("system.log", "a");
    fprintf(log_file, "[%s] %s\n", timestamp, action);
    fclose(log_file);
}
```

2. Display Hunter Info 
```
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
```
Fungsinya untuk menampilkan informasi semua hunter (nama, level, exp, atk, hp, def, banned/unbanned).

3. Generate Dungeon
```
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
```
Fungsinya untuk membuat dungeon baru dengan nama acak, level minimum, dan reward, menyimpannya di shared memory.

4. Display Dungeon Info 
```
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
```
Fungsinya untuk menampilkan informasi semua dungeon (nama, level minimum, reward, key). 

5. Ban Hunter 
```
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
```
Fungsinya untuk memban atau membuka ban hunter tertentu.

6. Reset Hunter 
```
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
```
Fungsinya untuk mereset stats hunter ke nilai awal.

7. Clean up
```
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
```
Fungsinya untuk membersihkan semua shared memory dan semaphore saat program dihentikan.

8. Main 
```
int main() {
    srand(time(NULL));
    log_file = fopen("system.log", "a");
    if (!log_file) {
        perror("fopen log");
        exit(1);
    }

    sem_unlink("/hunter_sem");
    sem = sem_open("/hunter_sem", O_CREAT, 0644, 1);
    key_t key = get_system_key();
    shmid = shmget(key, sizeof(struct SystemData), IPC_CREAT | 0666);
    struct SystemData *data = (struct SystemData *)shmat(shmid, NULL, 0);

    data->num_hunters = 0;
    data->num_dungeons = 0;
    data->current_notification_index = 0;
    log_action("System started");

    signal(SIGINT, cleanup);
    signal(SIGTERM, cleanup);

    while (1) {
        // Tampilkan menu admin
        // Panggil fungsi sesuai pilihan
    }
}
```
Fungsinya untuk mengatur alur utama program server, menyediakan menu admin.

# hunter.c

1. Log Action
```
void log_action(const char *action) {
    time_t now = time(NULL);
    char *timestamp = ctime(&now);
    timestamp[strlen(timestamp) - 1] = '\0';
    log_file = fopen("hunter.log", "a");
    fprintf(log_file, "[%s] %s\n", timestamp, action);
    fclose(log_file);
}
```
Fungsinya untuk mencatat aksi (misalnya, registrasi, login, raid) ke file hunter.log dengan timestamp.

2. Notification Thread
```
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
```
Fungsinya untuk menjalankan thread yang menampilkan notifikasi tentang dungeon yang tersedia setiap 3 detik, jika fitur notifikasi diaktifkan.

3. List Dungeons 
```
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
```
Fungsi ini untuk menampilkan daftar dungeon yang tersedia untuk hunter berdasarkan level mereka.

4. Raid  Dungeon 
```
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
```
Fungsi ini untuk memungkinkan hunter untuk menaklukkan dungeon yang sesuai dengan level mereka, mendapatkan reward, dan menghapus dungeon dari sistem.

5. Battle Hunter
```
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
```
Fungsi ini untuk memungkinkan hunter untuk bertarung dengan hunter lain, dengan pemenang mengambil stats lawan dan yang kalah dihapus dari sistem.

6. Main 
```
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
    // Menu utama
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
            // Registrasi
        } else if (choice == 2) {
            // Login dan menu hunter
        } else {
            printf("%s\n[ERROR] Invalid option.%s\n", RED, RESET);
            log_action("Invalid main menu option");
        }
    }
}
```
Fungsi ini untuk mengatur alur utama program client, menyediakan menu untuk registrasi, login, dan aksi hunter (list, raid, battle, notifikasi).


