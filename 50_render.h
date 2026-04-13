#ifndef RENDER_H
#define RENDER_H
#include <stdio.h>
#include <stdlib.h>
#include "10_data.h"
#include "51_render_helper.h"

void render(){
    if(!render_flag) return;
    render_flag=0;
    clear_screen(&window);

    if(game_state == STATE_NET_LOBBY){ draw_full_scr_overlay(0x0A0F1AFF); draw_stars(); lan_draw_lobby(&lan); update_screen(&window); return; } // LAN lobby screen
    if(game_state==STATE_MENU){        draw_full_scr_overlay(0x0A0F1AFF); draw_stars(); draw_menu();          update_screen(&window); return; } // menu screen

    draw_background();
    draw_terrain();
    draw_coins();
    draw_chests();
    draw_items();
    draw_decors();
    draw_pobjs();
    draw_enemies(ENEMY_CFG);
    //draw_player_sensors();
    draw_player();
    draw_p2();           // no-op when lan.role == LAN_OFF
    draw_projectiles();
    draw_hud();
    if(game_state==STATE_WIN){ draw_full_scr_overlay(0x00000099); draw_win_screen(); }
    if(game_state==STATE_PAUSE){ draw_full_scr_overlay(0x00000099); draw_pause_screen(); }
    update_screen(&window);
}

#endif