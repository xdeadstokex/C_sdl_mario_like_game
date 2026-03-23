#ifndef DATA_H
#define DATA_H

#include "external_lib/sdl2_wrapper.h"
#include "external_lib/io_logic.h"
#include "external_lib/time_util.h"
#include "external_lib/physic.h"
#include "external_lib/game_obj.h"

//###############################################
// FIXED STRUCTURAL CONSTANTS  (not tunable)
//###############################################
#define TERRAIN_COUNT  240
#define COIN_COUNT     30

#define ITEM_COUNT     3
#define CHEST_COUNT    15 //chest nums
#define PROJ_COUNT     50 //fireball nums

#define ENEMY_COUNT    50
#define DECOR_COUNT    64
#define POBJ_COUNT     16

#define STATE_MENU     0
#define STATE_PLAY     1
#define STATE_WIN      2
#define STATE_PAUSE    3

//###############################################
// SYSTEM DATA
//###############################################
struct cpu_window_data window;
struct mouse_data mouse;
struct kb_data kb;

//###############################################
// APP CONTROL
//###############################################
int render_flag;
timer_data tps_timer;

//###############################################
// GAME CONFIG  (loaded from world.txt, runtime)
//###############################################
struct game_config cfg;

//###############################################
// GAME DATA
//###############################################
struct camera_data camera;
struct player_data player;
struct terrain_data terrains[TERRAIN_COUNT];
struct coin_data coins[COIN_COUNT];
struct item_data items[ITEM_COUNT];
struct enemy_data enemies[ENEMY_COUNT];
struct decor_data decors[DECOR_COUNT];
struct chest_data chests[CHEST_COUNT];
struct projectile_data projectiles[PROJ_COUNT];
struct pobj_data  pobjs[POBJ_COUNT];
struct {
    sound_data jump;
    sound_data walk;
    sound_data dash;
    sound_data buff;
    sound_data chest;
    sound_data hit;
    sound_data hitted;
    sound_data die;
    sound_data win;
    sound_data coin;
    sound_data theme;
    sound_data fireball;
    sound_data bgm_menu;
    sound_data bgm_play_low_layer;
    sound_data bgm_play_high_layer;
} sfx;
int game_state;
int menu_sub_state;
int terrain_count_actual;
int enemy_count_actual;
int coin_count_actual;
int item_count_actual;
int decor_count_actual;
int chest_count_actual;
int pobj_count_actual;

font_data font;

#endif
