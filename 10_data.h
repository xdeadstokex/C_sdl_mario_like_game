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
#define STATE_NET_LOBBY 4

// include here since lan_game depend on above macros
// (moved to bottom — lan_game.h uses globals defined here, no externs needed)

//###############################################
// SYSTEM DATA
//###############################################
struct cpu_window_data window;
struct mouse_data mouse;
struct kb_data kb;
font_data font;
// included here to read all above system data
#include "external_lib/gui_util.h"
//###############################################
// APP CONTROL
//###############################################
int render_flag;
timer_data tps_timer;

//###############################################
// GAME CONFIG  (loaded from world.txt, runtime)
//###############################################
struct game_config cfg;
static enemy_cfg_t ENEMY_CFG[ENEMY_TYPE_COUNT];

//-----------------------------------------------
// GAME CONSTANTS (engine / shared)
//-----------------------------------------------
#define PROJ_TYPE_ENEMY        1

#define MAGE_RANGE_BASE        6.0
#define MAGE_RANGE_SCALE_Y     0.7
#define MAGE_RANGE_MIN         2.0
#define MAGE_RANGE_MAX         14.0
#define MAGE_DY_MIN           -8.0
#define MAGE_DY_MAX            12.0

#define TERRAIN_HIT_EPS_X      0.05
#define TERRAIN_HIT_Y_MIN     -0.1
#define TERRAIN_HIT_Y_MAX      0.2

#define PATROL_SPEED           1.0

#define DASH_FREQ_RAND         40
#define DASH_FREQ_OFFSET       20

#define DASH_TIME              12
#define DASH_TIME_DIV          20

#define DASH_PROJ_SPEED_BOSS   8.0

#define WEATHER_PLAT_COUNT     3

#define WEATHER_RESET_CD       60

#define STUN_TIME_DEAD         999999

//###############################################
// GAME CONSTANTS
//###############################################
#define REVIVE_OFFSET_Y        20.0
#define WORLD_EDGE_PAD         2.0

#define JUMP_BOUNCE_SCALE      0.7
#define PLAYER_MAX_HP          5
#define BREAK_TERRAIN_MAX_HP   5
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



gui_2d_data gui;
//###############################################
// LAN  — included last so it sees all globals above (no externs needed)
//###############################################
#include "external_lib/lan_game.h"
lan_ctx_t lan;

#endif