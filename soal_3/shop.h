#ifndef SHOP_H
#define SHOP_H

weapon *create(char *name, int damage, int price, char *passive);
void weapon_list();
void buy_weapon(player_state *player);

#endif 
