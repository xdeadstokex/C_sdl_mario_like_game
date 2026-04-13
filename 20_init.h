#ifndef INIT_H
#define INIT_H
#include <stdio.h>
#include <stdlib.h>
#include "10_data.h"
#include "external_lib/world_loader.h"
#include "41_process_helper.h"
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
    player.slow_timer = 0;
    player.hp = 5;
    player.fireball_ammo = 0;
    update_player_sensors();
    clear_sensor_flags();
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
    init_enemy_cfg(ENEMY_CFG);
    load_world(WORLD_FILE, 0, &cfg, ENEMY_CFG,
        terrains,&terrain_count_actual,TERRAIN_COUNT,
        enemies, &enemy_count_actual,  ENEMY_COUNT,
        coins,   &coin_count_actual,   COIN_COUNT,
        items,   &item_count_actual,   ITEM_COUNT,
        decors,  &decor_count_actual,  DECOR_COUNT,
        chests,  &chest_count_actual,  CHEST_COUNT,
        pobjs,   &pobj_count_actual,   POBJ_COUNT,
        &player.respawn_x, &player.respawn_y);

    cfg.screen_w = (double)window.w;
    cfg.screen_h = (double)window.h;

    for(int i=0;i<terrain_count_actual;i++){
        if(terrains[i].type==TERRAIN_BREAK){
            terrains[i].broken=0;
            terrains[i].broken_timer=0;
            terrains[i].hp=1;
        }
        terrains[i].warning_timer = 0;
    }
    double tps_scale = (double)cfg.tps / 20.0;
    cfg.dash_frames       = (int)(6  * tps_scale);
    cfg.invincible_frames = (int)(40 * tps_scale);

    player.score=0;
    reset_player();

    if(lan.role == LAN_HOST) lan_reset_p2(&lan);

    snap_camera();
}

//###############################################
// RELOAD WORLD  (K key — preserves player)
//###############################################
void reload_world(){
    double saved_px_per_m = cfg.px_per_m;
    init_enemy_cfg(ENEMY_CFG);
    load_world(WORLD_FILE, 1, &cfg, ENEMY_CFG,
        terrains,&terrain_count_actual,TERRAIN_COUNT,
        enemies, &enemy_count_actual,  ENEMY_COUNT,
        coins,   &coin_count_actual,   COIN_COUNT,
        items,   &item_count_actual,   ITEM_COUNT,
        decors,  &decor_count_actual,  DECOR_COUNT,
        chests,  &chest_count_actual,  CHEST_COUNT,
        pobjs,   &pobj_count_actual,   POBJ_COUNT,
        &player.respawn_x, &player.respawn_y);

    cfg.px_per_m = saved_px_per_m;
    cfg.screen_w = (double)window.w;
    cfg.screen_h = (double)window.h;

    for(int i=0;i<terrain_count_actual;i++){
        if(terrains[i].type==TERRAIN_BREAK){
            terrains[i].broken=0;
            terrains[i].broken_timer=0;
            terrains[i].hp=1;
        }
        terrains[i].warning_timer = 0;
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
    int target_tps = 60;
    reset_timer(&tps_timer, 1.0/target_tps);
    cfg.tps = target_tps;

    load_img(&window, &font.data, "resource/font_ASCII.png");
    set_font(&font, 5, 1);

    game_state=STATE_MENU;
    menu_sub_state=0;

    player.respawn_x=10.0;
    player.respawn_y=138.0;

    if (Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 2048) < 0) {
        printf("SDL_mixer Error: %s\n", Mix_GetError());
    }

    load_sound(&sfx.jump,   "assets/jump.mp3");
    load_sound(&sfx.dash,   "assets/dash.mp3");
    load_sound(&sfx.buff,   "assets/buff.mp3");
    load_sound(&sfx.chest,  "assets/chest.mp3");
    load_sound(&sfx.hit,    "assets/hit.mp3");
    load_sound(&sfx.hitted, "assets/hitted.mp3");
    load_sound(&sfx.die,    "assets/die.mp3");
    load_sound(&sfx.win,    "assets/win.mp3");
    load_sound(&sfx.coin,   "assets/coin.mp3");
    load_sound(&sfx.theme,   "assets/theme.mp3");
    load_sound(&sfx.fireball, "assets/fireball.mp3");
    load_sound(&sfx.bgm_menu, "assets/menu.mp3");
    load_sound(&sfx.bgm_play_low_layer, "assets/low.mp3");
    load_sound(&sfx.bgm_play_high_layer, "assets/high.mp3");
    play_sound_loop(&sfx.bgm_menu);

    lan_init_ctx(&lan);

    reset_game();
	init_menu_rects(&gui, cfg.screen_w, cfg.screen_h);
    return 1;
}

#endif