#ifndef GAME_OBJ_H
#define GAME_OBJ_H

#include "physic.h"

//###############################################
// GAME OBJECT STRUCTS + CONFIG
//
// UNIT SYSTEM: all positions/sizes/velocities
// are in METERS (double). Rendering converts
// to pixels at the draw boundary:
//   screen_px = world_m * cfg.px_per_m - camera_px
//
// physic.h is unit-agnostic (pure math).
//###############################################


//###############################################
// TYPE CONSTANTS
//###############################################
#define TERRAIN_SOLID  0
#define TERRAIN_BREAK  1

#define ENEMY_TYPE_COUNT 6 // remember to change this when add/remove enemy type
#define ENEMY_DASHER   0
#define ENEMY_BOSS     1
#define ENEMY_SHOOTER  2
#define ENEMY_SWORD    3
#define ENEMY_MAGE     4
#define ENEMY_WEATHER_BOSS  5

//###############################################
// GAME CONFIG
// All tunable values. Loaded from world.txt V tag.
// cfg.screen_w/h set from window after init_window.
// cfg.world_w/h set from V tags in world.txt.
//###############################################
struct game_config {
    // render / world scale
    double px_per_m;        // pixels per meter (default 100)
    double world_w;         // world width  in meters
    double world_h;         // world height in meters
    double screen_w;        // screen width  in pixels (from window)
    double screen_h;        // screen height in pixels (from window)

    // player physics  (m/s, m/s²)
    double gravity;
    double move_speed;
    double move_accel;
    double move_friction;
    double jump_vy;
    double jump_boost_vy;
    double dash_speed;
    int    dash_frames;
    double edge_grab_slide;
    int    invincible_frames;
    int    tps;              // target ticks per second — used to scale frame counters

    // player size  (meters)
    double player_w;
    double player_h;

    // velocity caps  (m/s)
    double max_vx;
    double max_vy;
};

typedef struct {
    int max_hp;

    int shoot_cd;
    double proj_speed;

    int mage_warn;
    int mage_cd;

    int dash_freq_lowhp;
    int dash_freq;
    double dash_speed_mult_lowhp;
    double dash_speed_mult;

    int stun_time;
    int stun_time_boss;

    double push_force;
    double push_lift;
    int push_slow_time;

    int is_boss;
    int instant_recover;
    int teleport_on_hit;

    int weather_cd_base;
    int weather_cd_var;
    double weather_push_range_sq;
    double weather_push_scale;
    double weather_dash_speed;

    // physics / spawn
    double col_w, col_h;
    double dash_vx;
    int action_cd;

    // ===== VISUAL =====
    unsigned int col_base;
    unsigned int col_lowhp;
    unsigned int col_dash;

    double hpbar_w, hpbar_h;
    double hpbar_yoff;
    int draw_eyes;
    double eye_size;
    int cull_margin;
    int has_weather_fx;

} enemy_cfg_t;

static struct game_config game_config_default(){
    struct game_config c;
    c.px_per_m          = 100.0;
    c.world_w           = 20.0;
    c.world_h           = 140.0;
    c.screen_w          = 1200.0;  // overwritten after init_window
    c.screen_h          = 900.0;   // overwritten after init_window
    c.gravity           = 18.0;
    c.move_speed        = 2.8;
    c.move_accel        = 20.0;
    c.move_friction     = 0.80;
    c.jump_vy           = -7.0;
    c.jump_boost_vy     = -9.5;
    c.dash_speed        = 7.0;
    c.dash_frames       = 6;    // frames at 20 TPS — scaled at runtime by tps/20
    c.edge_grab_slide   = 0.5;  // per 20-TPS frame decay — scaled via pow() in movement
    c.invincible_frames = 40;   // frames at 20 TPS — scaled at runtime
    c.tps               = 20;   // default, overwritten in init
    c.player_w          = 0.28;
    c.player_h          = 0.36;
    c.max_vx            = 10.0;   // m/s cap
    c.max_vy            = 20.0;   // m/s cap (generous for fast falls)
    return c;
}

void init_enemy_cfg(enemy_cfg_t* cfg){
    enemy_cfg_t* e;

    // clear all
    for(int i=0;i<ENEMY_TYPE_COUNT;i++){
        cfg[i] = (enemy_cfg_t){0};
    }

    // SWORD
    e = &cfg[ENEMY_SWORD];
	e->max_hp = 2;
    e->instant_recover = 1;
    e->stun_time = 60;
    e->push_force = 12.0;
    e->push_lift  = -6.0;
    e->col_w = 0.40; e->col_h = 0.45;
    e->dash_vx = 2.5;
    e->action_cd = 90;

	e->col_base=0xA9A9A9FF;
	e->col_dash=0xFF7700FF;
	e->hpbar_w=0.5; e->hpbar_h=0.05; e->hpbar_yoff=0.16;
	e->draw_eyes=1; e->eye_size=0.07;
	e->cull_margin=100;
	e->has_weather_fx = 0;

    // MAGE
    e = &cfg[ENEMY_MAGE];
	e->max_hp = 4;
    e->mage_warn = 40;
    e->mage_cd   = 100;
    e->stun_time = 60;
    e->push_force = 3.0;
    e->push_lift  = -3.0;
    e->col_w = 0.32; e->col_h = 0.40;
    e->dash_vx = 2.0;
    e->action_cd = 150;

	e->col_base=0x8A2BE2FF;
	e->col_dash=0xFF7700FF;
	e->hpbar_w=0.8; e->hpbar_h=0.1; e->hpbar_yoff=0.16;
	e->draw_eyes=1; e->eye_size=0.07;
	e->cull_margin=100;
	e->has_weather_fx = 0;

    // SHOOTER
    e = &cfg[ENEMY_SHOOTER];
	e->max_hp = 1;
    e->shoot_cd = 60;
    e->proj_speed = 6.0;
    e->stun_time = 60;
    e->push_force = 4.0;
    e->push_lift  = -3.5;
    e->col_w = 0.32; e->col_h = 0.32;
    e->dash_vx = 1.5;
    e->action_cd = 120;

	e->col_base   = 0x32CD32FF;
	e->col_dash   = 0xFF7700FF;
	e->hpbar_w    = 0.5;
	e->hpbar_h    = 0.05;
	e->hpbar_yoff = 0.16;
	e->draw_eyes  = 1;
	e->eye_size   = 0.07;
	e->cull_margin = 100;
	e->has_weather_fx = 0;

    // DASHER
    e = &cfg[ENEMY_DASHER];
	e->max_hp = 1;
    e->dash_freq = 100;
    e->dash_speed_mult = 1.0;
    e->stun_time = 60;
    e->push_force = 4.0;
    e->push_lift  = -3.5;
    e->col_w = 0.32; e->col_h = 0.32;
    e->dash_vx = 3.5;
    e->action_cd = 90;

	e->col_base   = 0xCC3333FF;
	e->col_dash   = 0xFF7700FF;
	e->hpbar_w    = 0.5;
	e->hpbar_h    = 0.05;
	e->hpbar_yoff = 0.16;
	e->draw_eyes  = 1;
	e->eye_size   = 0.07;
	e->cull_margin = 100;
	e->has_weather_fx = 0;

    // BOSS
    e = &cfg[ENEMY_BOSS];
	e->max_hp = 10;
    e->is_boss = 1;
    e->dash_freq = 50;
    e->dash_freq_lowhp = 30;
    e->dash_speed_mult = 1.0;
    e->dash_speed_mult_lowhp = 1.5;
    e->stun_time = 60;
    e->stun_time_boss = 20;
    e->push_force = 15.0;
    e->push_lift  = -7.0;
    e->push_slow_time = 100;
    e->col_w = 0.80; e->col_h = 0.80;
    e->dash_vx = 7.0;
    e->action_cd = 45;

	e->col_base   = 0xFF2200FF;
	e->col_lowhp  = 0x8B0000FF;
	e->col_dash   = 0xFF7700FF;
	e->hpbar_w    = 0.8;
	e->hpbar_h    = 0.1;
	e->hpbar_yoff = 0.16;
	e->draw_eyes  = 1;
	e->eye_size   = 0.07;
	e->cull_margin = 100;
	e->has_weather_fx = 0;

    // WEATHER BOSS
    e = &cfg[ENEMY_WEATHER_BOSS];
	e->max_hp = 5;
    e->is_boss = 1;
    e->teleport_on_hit = 1;
    e->stun_time = 60;
    e->push_force = 8.0;
    e->push_lift  = -5.0;
    e->weather_cd_base = 200;
    e->weather_cd_var  = 200;
    e->weather_push_range_sq = 324.0;
    e->weather_push_scale    = 0.35;
    e->weather_dash_speed    = 20.0;
    e->col_w = 0.70; e->col_h = 0.70;
    e->dash_vx = 20.0;
    e->action_cd = 100;

	e->col_base=0x00BFFFFF;
	e->col_dash=0xFF7700FF;
	e->hpbar_w=0.8; e->hpbar_h=0.1; e->hpbar_yoff=0.16;
	e->draw_eyes=1; e->eye_size=0.07;
	e->cull_margin=1800;
	e->has_weather_fx=1;
}

//###############################################
// RENDER CONVERSION  (meters -> screen pixels)
// Inline so zero overhead, used in 51_render_helper
//###############################################
static inline int m2sx(double world_x_m, double cam_x_px, double px_per_m){
    return (int)(world_x_m * px_per_m - cam_x_px);
}
static inline int m2sy(double world_y_m, double cam_y_px, double px_per_m){
    return (int)(world_y_m * px_per_m - cam_y_px);
}
// size in meters to pixels
static inline int m2sp(double size_m, double px_per_m){
    return (int)(size_m * px_per_m);
}


//###############################################
// STRUCTS  (all coordinates/sizes in meters)
//###############################################
struct camera_data {
    double x_px;   // camera left edge in pixels
    double y_px;   // camera top edge  in pixels
};

struct terrain_data {
    struct physic_base_data base;   // x,y = center in meters
    int    type;
    int    hp;
    int    broken;
    int    broken_timer;
    int    coin_inside;
    int    warning_timer;
};

struct coin_data {
    double x, y;       // center in meters
    int    collected;
};

struct chest_data {
    double x, y;
    int state;
    int item_type;
    int show_key;
};

struct item_data {
    double x, y;       // center in meters
    int    active;
    int    respawn_timer;
    int type;
};

struct enemy_data {
    struct physic_base_data base;   // x,y = center in meters
    int    type;
    int    active;
    int    stun_timer;
    int    patrol_dir;
    int    patrol_timer;
    double patrol_x_min, patrol_x_max;  // meters
    double patrol_y;                     // meters
    int    action_timer;
    int    dashing;
    double dash_vx;    // m/s
    int    hp;
};

// Sensor percentages (of player w/h)
// L/R sensors: thin vertically, inset top+bottom
// U/D sensors: thin horizontally, inset left+right
#define SENSOR_LR_W_PCT  0.10   // thin — less likely to graze corners
#define SENSOR_LR_H_PCT  0.60   // inset top+bottom by 20% each side
#define SENSOR_UD_W_PCT  0.60   // inset left+right by 20% each side
#define SENSOR_UD_H_PCT  0.10   // thin

// 4 directional sensors derived from player position each frame.
// Each is a world-space center rect (x,y,w,h). No physics.
struct player_sensor_data {
    double lx, ly, lw, lh; int l_active;  // left
    double rx, ry, rw, rh; int r_active;  // right
    double ux, uy, uw, uh; int u_active;  // up
    double dx, dy, dw, dh; int d_active;  // down
};

struct player_data {
    struct physic_base_data base;
    struct player_sensor_data sensors;
    int    on_ground;
    int    on_wall;
    int    edge_grab;
    int    grab_wall_dir;
    int    jump_count;
    int    dash_ready;
    int    dashing;
    int    dash_dir;
    int    invincible;
    int    god_mode;
    double respawn_x, respawn_y;
    int    jump_boost_timer;
    int speed_boost_timer;
    int slow_timer;
    int    score;
    int    last_move_dir;
    int    hp;
    int    fireball_ammo;

    // buffered inputs — set by control(), consumed+cleared by process_helper
    int    input_move_left;
    int    input_move_right;
    int    input_move_up;       // god mode up (W)
    int    input_move_down;     // god mode down (S)
    int    input_jump_click;    // space click
    int    input_jump_hold;     // space hold  (god mode fly)
    int    input_dash;          // shift click
    int    input_shoot;         // F click
    int    input_interact;      // E click
    int    input_reload_world;  // K click
    int    input_wall_press;    // a or d held — used for edge-grab detection
    int    input_god_mode_toggle; // G click — one-shot, sent over net
};

struct decor_data {
    double x, y;           // center in meters (physic space, Y-flipped on load)
    double w, h;           // meters
    unsigned int color;    // 0xRRGGBBAA
    char   label[32];      // empty string = no text
    int    is_teleporter;  // 1 = draw animated cage instead of plain rect
};


// Pushable physics object. Loaded from world.txt.
// Uses physic_base_data for full physics (gravity, velocity, friction, restitution).
// friction/restitution set per-object from world.txt — stored in base.
struct pobj_data {
    struct physic_base_data base;  // x,y = center in meters
    int    active;
    int    on_ground;
    unsigned int color;            // 0xRRGGBBAA, for rendering
};

// Fireball projectile
struct projectile_data {
    double x, y;
    double vx, vy;
    int    active;
    int    dir;
    int    type; //0:player, 1:mob 
};

#endif