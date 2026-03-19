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
    if(game_state==STATE_MENU){ draw_menu(); update_screen(&window); return; }
    draw_background();
    draw_terrain();
    draw_coins();
    draw_chests();
    draw_items();
    draw_decors();
    draw_enemies();
    draw_player();
    draw_hud();
    if(game_state==STATE_WIN)   draw_win_screen();
    if(game_state==STATE_PAUSE) draw_pause_screen();
    update_screen(&window);
}

#endif
