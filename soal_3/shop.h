#ifndef SHOP_H
#define SHOP_H

#include "dungeon.h"

typedef struct {
    char name[256];
    int price;
    int damage;
    char passive[256];
} weapon;

weapon *create(char *name, int damage, int price, char *passive);
void weapon_list();
void buy_weapon(player_state *player);

#endif 
