#include "shop.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

weapon *create(char *name, int damage, int price, char *passive) {
    weapon *tmp = malloc(sizeof(weapon));
    if (!tmp) return NULL;

    strcpy(tmp->name, name);
    tmp->damage = damage;
    tmp->price = price;
    strcpy(tmp->passive, passive);

    return tmp;
}

void weapon_list() {
    printf("+----+----------------+--------+--------+-----------------------+\n");
    printf("| No | %-14s | %-6s | %-6s | %-21s |\n", "Name", "Damage", "Price", "Bonus");
    printf("+----+----------------+--------+--------+-----------------------+\n");
    printf("| 1  | Pentung        | 500    | 1000   | +50% Critical Damage  |\n");
    printf("| 2  | Cappucina      | 200    | 400    | +20% Magic Damage     |\n");
    printf("| 3  | Banana         | 150    | 200    | None                  |\n");
    printf("| 4  | Bombardilo     | 500    | 750    | None                  |\n");
    printf("| 5  | Sword          | 400    | 600    | +10% Critical Damage  |\n");
    printf("+----+----------------+--------+--------+-----------------------+\n");
}

void buy_weapon(player_state *player) {
    int choice;
    weapon *weapons[] = { 
        create("Pentung", 500, 1000, "+50% Critical Damage"),
        create("Cappucina", 200, 400, "+20% Magic Damage"),
        create("Banana", 150, 200, ""),
        create("Bombardilo", 500, 750, ""),
        create("Sword", 400, 600, "+10% Critical Damage"),
    };

    weapon_list();
    printf("[GOLD: %d] Choose weapon to buy [1-5]: ", player->gold);
    scanf("%d", &choice);

    if (choice < 1 || choice > 5) {
        printf("Invalid choice!\n");
        return;
    }

    weapon *selected = weapons[choice - 1];

    if (player->gold < selected->price) {
        printf("Not enough gold!\n");
        return;
    }

    for (int i = 0; i < MAX_WEAPONS; i++) {
        if (player->weapon_repository[i] == NULL) {
            player->weapon_repository[i] = selected;
            player->gold -= selected->price;
            printf("%s:%d bought %s!\n", player->ip, player->port, selected->name);
            return;
        }
    }

    printf("Weapon repository full!\n");
}
