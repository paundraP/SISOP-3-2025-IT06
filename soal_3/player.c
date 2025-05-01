#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <time.h>
#include <cjson/cJSON.h>
#include "repository.h"

#define SERVER_IP "127.0.0.1"
#define SERVER_PORT 8080
#define BUFFER_SIZE 8192
#define MAX_WEAPONS 5

void menu() {
    printf("===MAIN MENU===\n");
    printf("1. Show Player Stats\n");
    printf("2. Shop (Buy Weapon)\n");
    printf("3. View Inventory & Equip Weapon\n");
    printf("4. Battle Mode\n");
    printf("5. Exit Game\n");
    printf("Choose an option: ");
}

void player_stats(const char *json_str) {
    cJSON *json = cJSON_Parse(json_str);
    if (!json) {
        fprintf(stderr, "Invalid JSON\n");
        return;
    }

    int gold = cJSON_GetObjectItem(json, "gold")->valueint;
    const char *equipped_weapon = cJSON_GetObjectItem(json, "equipped_weapon")->valuestring;
    int damage = cJSON_GetObjectItem(json, "damage")->valueint;
    int kill = cJSON_GetObjectItem(json, "kill")->valueint;
    const char *passive = cJSON_GetObjectItem(json, "passive")->valuestring;

    printf("===YOUR STATISTIC===\n");
    printf("Gold: %d\nEquipped Weapon: %s\nDamage: %d\nKills: %d\nPassive: %s\n", gold, equipped_weapon, damage, kill, passive);

    cJSON_Delete(json);
}

int main() {
    struct sockaddr_in server_addr;
    int socket_fd, choice;

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
    
        char choice;
        snprintf(choice, sizeof(choice), "%d", choice);
    
        if (send(socket_fd, choice, strlen(choice), 0) < 0) {
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
            player_stats(buffer);
        } else if (choice == 5) {
            printf("%s\n", buffer);
            return 0;
        }
    }
    
}