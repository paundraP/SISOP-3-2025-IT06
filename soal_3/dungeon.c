#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <cjson/cJSON.h>
#include "dungeon.h"

#define PORT 8080
#define BUFFER_SIZE 8192

// logic for buying weapons
weapon *create(char *name, int damage, int price, char *passive);
char *weapon_shop_list(weapon **weapons);
char *buy_weapon(player_state *player, weapon **weapons, const char *weapon_name);

// constructor look-like untuk membuat "player"
player_state *create_player(int socket, const char *ip, int port) {
    player_state *p = malloc(sizeof(player_state));
    if (!p) return NULL;

    p->socket = socket;
    strncpy(p->ip, ip, INET_ADDRSTRLEN);
    p->port = port;
    p->gold = 500;
    p->kill = 0;

    weapon *w = malloc(sizeof(weapon));
    if (!w) {
        free(p);
        return NULL; 
    }

    strcpy(w->name, "Knife");
    w->damage = 10;
    w->price = 0;
    strcpy(w->passive, "None");

    p->equipped_weapon = w;

    for (int i = 0; i < MAX_WEAPONS; i++) {
        p->weapon_repository[i] = NULL;
    }
    p->weapon_repository[0] = w;

    return p;
}


char* player_stats(player_state *player) {
    cJSON *json = cJSON_CreateObject();
    cJSON_AddNumberToObject(json, "gold", player->gold);
    cJSON_AddStringToObject(json, "equipped_weapon", player->equipped_weapon->name);
    cJSON_AddNumberToObject(json, "damage", player->equipped_weapon->damage);
    cJSON_AddNumberToObject(json, "kill", player->kill);
    cJSON_AddStringToObject(json, "passive", player->equipped_weapon->passive);

    char *json_str = cJSON_PrintUnformatted(json);
    cJSON_Delete(json);
    return json_str;
}

void* client_handler(void* arg) {
    player_state* player = (player_state*)arg;
    weapon *weapons[] = { 
        create("Pentung", 500, 1000, "+50% Critical Damage"),
        create("Cappucina", 200, 400, "+20% Magic Damage"),
        create("Banana", 150, 200, ""),
        create("Bombardilo", 500, 750, ""),
        create("Sword", 400, 600, "+10% Critical Damage"),
    };
    char cmd[8];
    int connected = 1;

    while (connected) {
        memset(cmd, 0, sizeof(cmd));
        int recv_len = recv(player->socket, cmd, sizeof(cmd) - 1, 0);
        if (recv_len <= 0) break;

        if(strcmp(cmd, "1") == 0) {
            char *response = player_stats(player);
            send(player->socket, response, strlen(response), 0);
            free(response);
        } else if (strcmp(cmd, "2") == 0) {
            char *json = weapon_shop_list(weapons);
            send(player->socket, json, strlen(json), 0);
            free(json);

            char weapon_name[BUFFER_SIZE];
            int recv_len = recv(player->socket, weapon_name, sizeof(weapon_name), 0);
            if (recv_len == 0) {
                perror("recv failed or connection closed");
                return NULL;
            }

            char *response = buy_weapon(player, weapons, weapon_name);
            send(player->socket, response, strlen(response), 0);

            free(response);
        } else if (strcmp(cmd, "5") == 0) {
            char *response = "Bye!";
            connected = 0;
            send(player->socket, response, strlen(response), 0);
            break;
        }
    }
    printf("[-] Disconnected: %s:%d\n", player->ip, player->port);
    close(player->socket);
    free(player);
    pthread_exit(NULL);
}

int main() {
    int server_fd, client_socket;
    struct sockaddr_in addr;
    socklen_t addrlen = sizeof(addr);

    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    int opt = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(PORT);

    bind(server_fd, (struct sockaddr*)&addr, sizeof(addr));
    listen(server_fd, 10);

    printf("Server listening on port %d...\n", PORT);

    while (1) {
        client_socket = accept(server_fd, (struct sockaddr*)&addr, &addrlen);
        if (client_socket < 0) continue;

        char ip_str[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &addr.sin_addr, ip_str, INET_ADDRSTRLEN); 
        int client_port = ntohs(addr.sin_port);

        player_state* client = create_player(client_socket, ip_str, client_port);


        printf("[+] New connection from %s:%d\n", client->ip, client->port);

        pthread_t tid;
        pthread_create(&tid, NULL, client_handler, client);
        pthread_detach(tid);  // free thread resourse waktu exit
    }

    close(server_fd);
    return 0;
}
