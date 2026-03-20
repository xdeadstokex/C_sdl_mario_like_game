#ifndef INIT_H
#define INIT_H
#include <stdio.h>
#include <stdlib.h>
#include "10_data.h"
#include "external_lib/world_loader.h"

#define WORLD_FILE "resource/world.txt"

//###############################################
// PLAYER RESET
//###############################################
void reset_player(){
    set_physic_base(&player.base,
        player.respawn_x, player.respawn_y,
        0,0, 0,cfg.gravity, 60, 0.4,0.0,
        0,0, cfg.player_w, cfg.player_h);
    player.on_ground=0;  player.on_wall=0;
    player.edge_grab=0;  player.grab_wall_dir=0;
    player.jump_count=0; player.dash_ready=1;
    player.dashing=0;    player.dash_dir=1;
    player.invincible=0; player.god_mode=0;
    player.last_move_dir=1; player.jump_boost_timer=0;
    player.speed_boost_timer = 0;
    player.hp = 5;
    player.fireball_ammo = 0;

}

//###############################################
// SNAP CAMERA TO PLAYER
//###############################################
static void snap_camera(){
    double pm=cfg.px_per_m, sw=cfg.screen_w, sh=cfg.screen_h;
    camera.x_px = player.base.x*pm - sw/2.0;
    camera.y_px = player.base.y*pm - sh/2.0;
}

//###############################################
// RESET GAME  (new game / play again)
//###############################################
void reset_game(){
    load_world(WORLD_FILE, 0, &cfg,
        terrains,&terrain_count_actual,TERRAIN_COUNT,
        enemies, &enemy_count_actual,  ENEMY_COUNT,
        coins,   &coin_count_actual,   COIN_COUNT,
        items,   &item_count_actual,   ITEM_COUNT,
        decors,  &decor_count_actual,  DECOR_COUNT,
        chests,  &chest_count_actual,  CHEST_COUNT,
        pobjs,   &pobj_count_actual,   POBJ_COUNT,
        &player.respawn_x, &player.respawn_y);

    // screen dims come from window, not file
    cfg.screen_w = (double)window.w;
    cfg.screen_h = (double)window.h;

    for(int i=0;i<terrain_count_actual;i++){
        if(terrains[i].type==TERRAIN_BREAK){
            terrains[i].broken=0;
            terrains[i].broken_timer=0;
            terrains[i].hp=1;
        }
    }
    player.score=0;
    reset_player();
    snap_camera();
}

//###############################################
// RELOAD WORLD  (K key — preserves player)
//###############################################
void reload_world(){
    double saved_px_per_m = cfg.px_per_m;  // preserve zoom level

    load_world(WORLD_FILE, 1, &cfg,
        terrains,&terrain_count_actual,TERRAIN_COUNT,
        enemies, &enemy_count_actual,  ENEMY_COUNT,
        coins,   &coin_count_actual,   COIN_COUNT,
        items,   &item_count_actual,   ITEM_COUNT,
        decors,  &decor_count_actual,  DECOR_COUNT,
        chests,  &chest_count_actual,  CHEST_COUNT,
        pobjs,   &pobj_count_actual,   POBJ_COUNT,
        &player.respawn_x, &player.respawn_y);

    cfg.px_per_m = saved_px_per_m;         // restore zoom
    cfg.screen_w = (double)window.w;
    cfg.screen_h = (double)window.h;

    for(int i=0;i<terrain_count_actual;i++){
        if(terrains[i].type==TERRAIN_BREAK){
            terrains[i].broken=0;
            terrains[i].broken_timer=0;
            terrains[i].hp=1;
        }
    }
    player.base.ay = cfg.gravity;
}

//###############################################
// INIT
//###############################################
int init(){
    init_graphic_lib();
    init_window(&window, 1200, 800, "Vertical Climber");

    render_flag=0;
    reset_timer(&tps_timer, 1.0/25.0);

    load_img(&window, &font.data, "resource/font_ASCII.png");
    set_font(&font, 5, 1);

    game_state=STATE_MENU;
    menu_sub_state=0;

    // fallback spawn
    player.respawn_x=10.0;
    player.respawn_y=138.0;

    reset_game();
    return 1;
}

#endif
