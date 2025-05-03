#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <time.h>
#include <cjson/cJSON.h>
#include "dungeon.h"

#define SERVER_IP "127.0.0.1"
#define SERVER_PORT 8080 
#define BUFFER_SIZE 8192

typedef struct {
    int gold;
    char equipped_weapon[64];
    int damage;
    int kill;
    char passive[128];
    int exist;
} Player;

void menu() {
    printf("===MAIN MENU===\n");
    printf("1. Show Player Stats\n");
    printf("2. Shop (Buy Weapon)\n");
    printf("3. View Inventory & Equip Weapon\n");
    printf("4. Battle Mode\n");
    printf("5. Exit Game\n");
    printf("Choose an option: ");
}

void player_stats(const char *json_str, Player *p) {
    cJSON *json = cJSON_Parse(json_str);
    if (!json) {
        fprintf(stderr, "Invalid JSON\n");
        return;
    }

    p->gold = cJSON_GetObjectItem(json, "gold")->valueint;
    strncpy(p->equipped_weapon, cJSON_GetObjectItem(json, "equipped_weapon")->valuestring, sizeof(p->equipped_weapon));
    p->damage = cJSON_GetObjectItem(json, "damage")->valueint;
    p->kill = cJSON_GetObjectItem(json, "kill")->valueint;
    strncpy(p->passive, cJSON_GetObjectItem(json, "passive")->valuestring, sizeof(p->passive));
    p->exist = 1;

    printf("===YOUR STATISTIC===\n");
    printf("Gold: %d\nEquipped Weapon: %s\nDamage: %d\nKills: %d\nPassive: %s\n",
           p->gold, p->equipped_weapon, p->damage, p->kill, p->passive);

    cJSON_Delete(json);
}

void buy_weapon(Player *player, const char *json_str, int socket_fd) {
    cJSON *json = cJSON_Parse(json_str);
    if (!json) {
        fprintf(stderr, "Invalid JSON\n");
        return;
    }

    int size = cJSON_GetArraySize(json);
    printf("=== WEAPON SHOP ===\nYour gold: %d\n", player->gold);

    for (int i = 0; i < size; i++) {
        cJSON *w = cJSON_GetArrayItem(json, i);
        const char *name = cJSON_GetObjectItem(w, "name")->valuestring;
        int damage = cJSON_GetObjectItem(w, "damage")->valueint;
        int price = cJSON_GetObjectItem(w, "price")->valueint;
        const char *passive = cJSON_GetObjectItem(w, "passive")->valuestring;

        printf("[%d] %s (Damage: %d, Price: %d, Passive: %s)\n", i + 1, name, damage, price, passive);
    }

    int choice;
    printf("Choose weapon number to buy: ");
    scanf("%d", &choice);
    getchar();

    if (choice < 1 || choice > size) {
        printf("Invalid choice.\n");
        cJSON_Delete(json);
        return;
    }

    cJSON *weapon = cJSON_GetArrayItem(json, choice - 1);
    const char *weapon_name = cJSON_GetObjectItem(weapon, "name")->valuestring;

    send(socket_fd, weapon_name, strlen(weapon_name), 0);

    char response[BUFFER_SIZE] = {0};
    recv(socket_fd, response, sizeof(response), 0);
    printf("%s\n", response);

    cJSON_Delete(json);
}

void show_inventory(const char *json_str, int socket_fd) {
    cJSON *json = cJSON_Parse(json_str);
    if (!json) {
        fprintf(stderr, "Invalid JSON\n");
        return;
    }

    int array_size = cJSON_GetArraySize(json);
    if (array_size == 0) {
        printf("Inventory is empty.\n");
        cJSON_Delete(json);
        return;
    }

    printf("=== Your Inventory ===\n");
    for (int i = 0; i < array_size; i++) {
        cJSON *item = cJSON_GetArrayItem(json, i);
        cJSON *name = cJSON_GetObjectItem(item, "name");
        cJSON *damage = cJSON_GetObjectItem(item, "damage");
        cJSON *price = cJSON_GetObjectItem(item, "price");
        cJSON *passive = cJSON_GetObjectItem(item, "passive");

        if (name && damage && price && passive) {
            printf("%d. %s | Damage: %d | Price: %d | Passive: %s\n",
                   i + 1,
                   name->valuestring,
                   damage->valueint,
                   price->valueint,
                   passive->valuestring);
        }
    }

    int choice = 0;
    printf("Choose weapon number to equip: ");
    scanf("%d", &choice);

    if (choice < 1 || choice > array_size) {
        printf("Invalid choice.\n");
        cJSON_Delete(json);
        return;
    }

    // Get selected weapon name
    cJSON *selected = cJSON_GetArrayItem(json, choice - 1);
    cJSON *name = cJSON_GetObjectItem(selected, "name");
    printf("selected %s\n", name->valuestring);
    if (!name) {
        printf("Weapon name not found.\n");
        cJSON_Delete(json);
        return;
    }

    // Send weapon name to server
    send(socket_fd, name->valuestring, strlen(name->valuestring), 0);
    
    char response[BUFFER_SIZE] = {0};
    recv(socket_fd, response, sizeof(response), 0);
    printf("%s\n", response);

    cJSON_Delete(json);
}


int main() {
    struct sockaddr_in server_addr;
    int socket_fd, choice;
    Player player = {0};

    socket_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (socket_fd < 0) {
        perror("Failed to create socket");
        exit(EXIT_FAILURE);
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(SERVER_PORT);
    inet_pton(AF_INET, SERVER_IP, &server_addr.sin_addr);

    if (connect(socket_fd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("Failed to connect to server");
        close(socket_fd);
        exit(EXIT_FAILURE);
    }

    while (1) {
        menu();
        scanf("%d", &choice);
        getchar();
    
        char opt[8];
        snprintf(opt, sizeof(opt), "%d", choice);
    
        if (send(socket_fd, opt, strlen(opt), 0) < 0) {
            perror("Send failed");
            break;
        }
    
        char buffer[BUFFER_SIZE] = {0};
        int recv_len = recv(socket_fd, buffer, sizeof(buffer), 0);
        if (recv_len == 0) {
            printf("Server closed the connection.\n");
            break;
        }

        if (choice == 1) {
            player_stats(buffer, &player);
        } else if (choice == 2)  {
            buy_weapon(&player, buffer, socket_fd);
        } else if (choice == 3) {
            show_inventory(buffer, socket_fd);
        } else if (choice == 5) {
            printf("%s\n", buffer);
            return 0;
        }
    }
}