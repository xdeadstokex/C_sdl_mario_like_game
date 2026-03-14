#ifndef DATA_H
#define DATA_H

#include "external_lib/sdl2_wrapper.h"
#include "external_lib/io_logic.h"
#include "external_lib/time_util.h"
#include "external_lib/physic.h"

//###############################################
// CONSTANTS
//###############################################
#define WORLD_W           2000
#define WORLD_H           14000
#define SCREEN_W          1200
#define SCREEN_H          900

#define GRAVITY           1800.0
#define MOVE_SPEED        280.0
#define MOVE_ACCEL        2000.0
#define MOVE_FRICTION     0.80
#define JUMP_VY           -700.0
#define JUMP_BOOST_VY     -950.0
#define DASH_SPEED        700.0
#define DASH_FRAMES       6
#define EDGE_GRAB_SLIDE   0.5
#define INVINCIBLE_FRAMES 40

#define TERRAIN_COUNT     240
#define COIN_COUNT        30
#define ITEM_COUNT        2
#define ENEMY_COUNT       7

#define TERRAIN_SOLID     0
#define TERRAIN_BREAK     1
#define ENEMY_DASHER      0
#define ENEMY_BOSS        1

#define STATE_MENU        0
#define STATE_PLAY        1
#define STATE_WIN         2
#define STATE_PAUSE       3

//###############################################
// STRUCTS
//###############################################
struct camera_data {
    double x;
    double y;
};

struct terrain_data {
    struct physic_base_data base;
    int type;
    int hp;
    int broken;
    int broken_timer;
    int coin_inside;
};

struct coin_data {
    double x, y;
    int collected;
};

struct item_data {
    double x, y;
    int active;
    int respawn_timer;
};

struct enemy_data {
    struct physic_base_data base;
    int type;
    int active;
    int stun_timer;
    int patrol_dir;
    int patrol_timer;
    double patrol_x_min, patrol_x_max;
    double patrol_y;
    int dash_timer;
    int dashing;
    double dash_vx;
    int hp;
};

struct player_data {
    struct physic_base_data base;
    int on_ground;
    int on_wall;
    int edge_grab;
    int grab_wall_dir;
    int jump_count;
    int dash_ready;
    int dashing;
    int dash_dir;
    int invincible;
    int god_mode;
    double respawn_x, respawn_y;
    int jump_boost_timer;
    int score;
    int last_move_dir;
};

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
// GAME DATA
//###############################################
struct camera_data camera;
struct player_data player;
struct terrain_data terrains[TERRAIN_COUNT];
struct coin_data coins[COIN_COUNT];
struct item_data items[ITEM_COUNT];
struct enemy_data enemies[ENEMY_COUNT];

int game_state;
int menu_sub_state;
int terrain_count_actual;

font_data font;

#endif
