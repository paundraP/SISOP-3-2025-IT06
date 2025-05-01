#ifndef DUNGEON_H
#define DUNGEON_H

#include <netinet/in.h>

#define MAX_WEAPONS 5

typedef struct {
    char name[256];
    int price;
    int damage;
    char passive[256];
} weapon;

typedef struct {
    int socket;
    char ip[INET_ADDRSTRLEN];
    int port;
    int gold;
    weapon *equipped_weapon;
    weapon *weapon_repository[MAX_WEAPONS];
    int kill; 
} player_state;

player_state *create_player(int socket, const char *ip, int port);
char* player_stats(player_state *player);

#endif
