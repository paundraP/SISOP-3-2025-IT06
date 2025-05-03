#include "dungeon.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <cjson/cJSON.h>

weapon *create(char *name, int damage, int price, char *passive) {
    weapon *tmp = malloc(sizeof(weapon));
    if (!tmp) return NULL;

    strcpy(tmp->name, name);
    tmp->damage = damage;
    tmp->price = price;
    strcpy(tmp->passive, passive);

    return tmp;
}

char *weapon_shop_list(weapon **weapons) {
    cJSON *weapons_json = cJSON_CreateArray();

    for (int i = 0; i < 5; i++) {
        cJSON *w = cJSON_CreateObject();
        cJSON_AddStringToObject(w, "name", weapons[i]->name);
        cJSON_AddNumberToObject(w, "damage", weapons[i]->damage);
        cJSON_AddNumberToObject(w, "price", weapons[i]->price);
        cJSON_AddStringToObject(w, "passive", strlen(weapons[i]->passive) ? weapons[i]->passive : "None");
        cJSON_AddItemToArray(weapons_json, w);
    }

    char *json_str = cJSON_PrintUnformatted(weapons_json);
    cJSON_Delete(weapons_json);
    return json_str;
}


char *buy_weapon(player_state *player, weapon **weapons, char *weapon_name) {
    weapon *selected = NULL;
    for (int i = 0; i < MAX_WEAPONS; i++) {
        if (strcmp(weapon_name, weapons[i]->name) == 0) {
            selected = malloc(sizeof(weapon));
            if (!selected) return strdup("Memory allocation failed.\n");

            // Copy name and passive safely using strncpy
            strncpy(selected->name, weapons[i]->name, sizeof(selected->name) - 1);
            selected->name[sizeof(selected->name) - 1] = '\0';  // Ensure null-termination

            strncpy(selected->passive, weapons[i]->passive, sizeof(selected->passive) - 1);
            selected->passive[sizeof(selected->passive) - 1] = '\0';  // Ensure null-termination

            selected->damage = weapons[i]->damage;
            selected->price = weapons[i]->price;
            break;
        }
    }

    if (!selected) {
        return strdup("Weapon not found.\n");
    }

    if (player->gold < selected->price) {
        free(selected);
        return strdup("Not enough gold!\n");
    }

    for (int i = 0; i < MAX_WEAPONS; i++) {
        if (player->weapon_repository[i] == NULL) {
            player->weapon_repository[i] = selected;
            player->gold -= selected->price;
            char *response = malloc(128);
            snprintf(response, 128, "You successfully bought %s.\n", selected->name);
            printf("%s:%d bought %s!\n", player->ip, player->port, selected->name);
            return response;
        }
    }

    free(selected);
    return strdup("Weapon repository full!\n");
}
