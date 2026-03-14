#ifndef RENDER_HELPER_H
#define RENDER_HELPER_H
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "10_data.h"
#include "external_lib/physic.h"

// world to screen
static inline int sx(double wx){ return (int)(wx - camera.x); }
static inline int sy(double wy){ return (int)(wy - camera.y); }

//###############################################
// SKY  (fix #6/#7: based on player world y, mountain style)
// y=0 summit, y=14000 base
//###############################################
static unsigned int lerp_color(unsigned int c0, unsigned int c1, double t){
    if(t < 0) t=0; if(t > 1) t=1;
    int r0=(c0>>24)&0xFF, g0=(c0>>16)&0xFF, b0=(c0>>8)&0xFF;
    int r1=(c1>>24)&0xFF, g1=(c1>>16)&0xFF, b1=(c1>>8)&0xFF;
    int r=(int)(r0+(r1-r0)*t);
    int g=(int)(g0+(g1-g0)*t);
    int b=(int)(b0+(b1-b0)*t);
    return ((unsigned int)r<<24)|((unsigned int)g<<16)|((unsigned int)b<<8)|0xFF;
}

void draw_background(){
    double py = player.base.y;  // fix #6: actual player y, not camera

    // color stops: bottom=dark rock, top=bright summit sky
    // y: 14000 -> 11000 -> 8000 -> 5000 -> 2000 -> 0
    unsigned int c_base    = 0x1A1214FF; // dark cave/base rock
    unsigned int c_lower   = 0x2D2B3DFF; // lower mountain dusk purple
    unsigned int c_mid     = 0x3D5A80FF; // mid mountain blue gray
    unsigned int c_upper   = 0x7090B0FF; // upper mountain sky
    unsigned int c_summit  = 0xC8DCF0FF; // near summit, bright cold sky

    unsigned int bg;
    if(py > 11000){
        double t = (py - 11000.0) / 3000.0;
        bg = lerp_color(c_lower, c_base, t);
    } else if(py > 8000){
        double t = (py - 8000.0) / 3000.0;
        bg = lerp_color(c_mid, c_lower, t);
    } else if(py > 5000){
        double t = (py - 5000.0) / 3000.0;
        bg = lerp_color(c_upper, c_mid, t);
    } else if(py > 2000){
        double t = (py - 2000.0) / 3000.0;
        bg = lerp_color(c_summit, c_upper, t);
    } else {
        bg = c_summit;
    }

    draw_rect(&window, 0, 0, SCREEN_W, SCREEN_H, bg);

    // distant mountain silhouettes (parallax layer, fixed screen-space shapes)
    // only visible in lower zones
    if(py > 4000){
        double para = 0.3; // parallax factor
        int mx = (int)(camera.x * para);
        unsigned int mtn_col = lerp_color(0x444450FF, bg, 0.5);
        // two big mountain triangles approximated with stacked rects
        for(int row=0; row<20; row++){
            int rw = 300 - row*14;
            int rx = 100 - mx % SCREEN_W - rw/2 + row*7;
            int ry = SCREEN_H - 300 + row*13;
            if(ry > 0 && ry < SCREEN_H)
                draw_rect(&window, rx, ry, rw > 0 ? rw : 0, 14, mtn_col);
        }
        for(int row=0; row<20; row++){
            int rw = 280 - row*13;
            int rx = 800 - mx % SCREEN_W - rw/2 + row*6;
            int ry = SCREEN_H - 280 + row*12;
            if(ry > 0 && ry < SCREEN_H)
                draw_rect(&window, rx, ry, rw > 0 ? rw : 0, 13, mtn_col);
        }
    }
}

//###############################################
// TERRAIN  (fix #7: mountain colors)
//###############################################
void draw_terrain(){
    for(int i=0; i<terrain_count_actual; i++){
        if(terrains[i].broken) continue;

        double tx = terrains[i].base.x;
        double ty = terrains[i].base.y;
        double tw = terrains[i].base.col_w;
        double th = terrains[i].base.col_h;

        int dsx = sx(tx - tw/2);
        int dsy = sy(ty - th/2);
        int dsw = (int)tw;
        int dsh = (int)th;

        if(dsy+dsh < 0 || dsy > SCREEN_H) continue;
        if(dsx+dsw < 0 || dsx > SCREEN_W) continue;

        if(terrains[i].type == TERRAIN_BREAK){
            draw_rect(&window, dsx, dsy, dsw, dsh, 0x9B6B4BFF);
            // crack lines
            draw_line(&window, dsx+4, dsy+4, dsx+dsw-4, dsy+dsh-4, 0x5A3A2AFF);
            draw_line(&window, dsx+dsw-4, dsy+4, dsx+4, dsy+dsh-4, 0x5A3A2AFF);
        } else {
            // snow by altitude (fix #7)
            unsigned int rock_col;
            if(ty < 3000)       rock_col = 0xDCDCECFF; // snow white
            else if(ty < 7000)  rock_col = 0x8A8A9AFF; // gray rock
            else                rock_col = 0x5A5A6AFF; // dark slate base

            draw_rect(&window, dsx, dsy, dsw, dsh, rock_col);

            // snow cap: white top edge on upper terrain
            unsigned int top_col = (ty < 3000) ? 0xFFFFFFFF :
                                   (ty < 7000) ? 0xBBBBCCFF : 0x777788FF;
            draw_rect(&window, dsx, dsy, dsw, 3, top_col);

            // slight shadow on bottom edge for depth
            draw_rect(&window, dsx, dsy+dsh-3, dsw, 3, 0x00000033);
        }
    }
}

//###############################################
// PLAYER
//###############################################
void draw_player(){
    int spx = sx(player.base.x);
    int spy = sy(player.base.y);
    int pw  = (int)player.base.col_w;
    int ph  = (int)player.base.col_h;

    if(player.invincible > 0 && (player.invincible/3)%2 == 0) return;

    // body color: yellow during boost, teal during grab, blue normal
    unsigned int body_col = 0x4169E1FF;
    if(player.edge_grab)          body_col = 0x00CED1FF;
    if(player.jump_boost_timer>0) body_col = 0xFFAA00FF;

    draw_rect_centered(&window, spx, spy, pw, ph, body_col);

    // head
    draw_rect_centered(&window, spx, spy - ph/2 - 8, 20, 18, 0xFFD700FF);
    // eyes
    int ex = (player.last_move_dir > 0) ? spx + 4 : spx - 4;
    draw_rect_centered(&window, ex, spy - ph/2 - 10, 5, 5, 0x222222FF);

    // dash trail
    if(player.dashing > 0){
        for(int t=1; t<=3; t++){
            int tx2 = spx - player.dash_dir * t * 14;
            unsigned int tc = 0xFFFFFF00 | (unsigned int)(60 - t*16);
            draw_rect_centered(&window, tx2, spy, pw - t*5, ph - t*5, tc);
        }
    }

    // grab indicator: small hand marks on wall side
    if(player.edge_grab){
        int hand_x = spx + player.grab_wall_dir * (pw/2 + 3);
        draw_rect_centered(&window, hand_x, spy - 6, 6, 5, 0xFFD700FF);
        draw_rect_centered(&window, hand_x, spy + 6, 6, 5, 0xFFD700FF);
    }
}

//###############################################
// ENEMIES
//###############################################
void draw_enemies(){
    for(int i=0; i<ENEMY_COUNT; i++){
        struct enemy_data* e = &enemies[i];
        int esx = sx(e->base.x);
        int esy = sy(e->base.y);
        if(esy < -100 || esy > SCREEN_H+100) continue;

        int ew = (int)e->base.col_w;
        int eh = (int)e->base.col_h;

        if(!e->active){
            draw_rect_centered(&window, esx, esy, ew, eh, 0x666666FF);
            // X eyes
            draw_line(&window, esx-ew/5, esy-eh/5, esx+ew/5, esy+eh/5, 0x222222FF);
            draw_line(&window, esx+ew/5, esy-eh/5, esx-ew/5, esy+eh/5, 0x222222FF);
            continue;
        }

        unsigned int col = (e->type==ENEMY_BOSS) ? 0xFF2200FF : 0xCC3333FF;
        if(e->dashing) col = 0xFF7700FF;
        draw_rect_centered(&window, esx, esy, ew, eh, col);

        // eyes
        int eye_y = esy - eh/5;
        draw_rect_centered(&window, esx-ew/5, eye_y, ew/7, ew/7, 0xFFFF00FF);
        draw_rect_centered(&window, esx+ew/5, eye_y, ew/7, ew/7, 0xFFFF00FF);

        // boss HP bar
        if(e->type==ENEMY_BOSS && e->hp>0){
            int bw=80, bh=10;
            int bx=esx-bw/2, by=esy-eh/2-16;
            draw_rect(&window, bx,   by, bw, bh, 0x333333FF);
            draw_rect(&window, bx,   by, bw*e->hp/5, bh, 0xFF3333FF);
            draw_rect(&window, bx,   by, bw, 2, 0xFFFFFF44);
        }
    }
}

//###############################################
// COINS
//###############################################
void draw_coins(){
    for(int i=0; i<COIN_COUNT; i++){
        if(coins[i].collected) continue;
        int csx = sx(coins[i].x);
        int csy = sy(coins[i].y);
        if(csy<-20||csy>SCREEN_H+20) continue;
        draw_rect_centered(&window, csx, csy, 16, 16, 0xFFD700FF);
        draw_rect_centered(&window, csx, csy,  9,  9, 0xFFF5B0FF);
    }
}

//###############################################
// ITEMS
//###############################################
void draw_items(){
    for(int i=0; i<ITEM_COUNT; i++){
        if(!items[i].active) continue;
        int isx = sx(items[i].x);
        int isy = sy(items[i].y);
        if(isy<-20||isy>SCREEN_H+20) continue;
        // star: two overlapping rects at offsets
        draw_rect_centered(&window, isx, isy, 22, 8,  0xFFFF00FF);
        draw_rect_centered(&window, isx, isy, 8,  22, 0xFFFF00FF);
        draw_rect_centered(&window, isx, isy, 14, 14, 0xFFAA00FF);
        draw_rect_centered(&window, isx, isy,  6,  6, 0xFFFFFFFF);
    }
}

//###############################################
// WIN DECOR + TELEPORTER CAGE
// Summit platform at y=300, win platform x=600-1400
// Teleporter cage: center (1000,260), 80x80
//###############################################
void draw_win_decor(){
    // only draw if summit area is visible
    int summit_sy = sy(270);
    if(summit_sy < -200 || summit_sy > SCREEN_H + 200) return;

    // --- tree (left of summit) ---
    int tree_sx = sx(680);
    int tree_sy = sy(300);
    draw_rect(&window, tree_sx-6, tree_sy-90, 12, 90, 0x6B4423FF);
    draw_rect_centered(&window, tree_sx, tree_sy-110, 60, 30, 0x2E7D32FF);
    draw_rect_centered(&window, tree_sx, tree_sy-130, 46, 28, 0x388E3CFF);
    draw_rect_centered(&window, tree_sx, tree_sy-148, 30, 24, 0x43A047FF);
    draw_rect_centered(&window, tree_sx, tree_sy-148, 26,  8, 0xEEEEEEFF);
    draw_rect_centered(&window, tree_sx, tree_sy-132, 38,  6, 0xEEEEEEFF);

    // --- coffee table (right of summit) ---
    int ct_sx = sx(1300);
    int ct_sy = sy(300);
    draw_rect_centered(&window, ct_sx,     ct_sy-12, 70, 14, 0xA0522DFF);
    draw_rect(&window, ct_sx-33, ct_sy-12,  7, 24, 0x8B4513FF);
    draw_rect(&window, ct_sx+26, ct_sy-12,  7, 24, 0x8B4513FF);
    draw_rect_centered(&window, ct_sx-10, ct_sy-22, 12, 14, 0xFFFFFFFF);
    draw_rect_centered(&window, ct_sx-10, ct_sy-22,  9, 10, 0x6B3A2AFF);

    // --- sign board ---
    int sg_sx = sx(870);
    int sg_sy = sy(265);
    draw_rect_centered(&window, sg_sx, sg_sy, 160, 32, 0xF5DEB3FF);
    draw_rect_centered(&window, sg_sx, sg_sy, 156, 28, 0xEDD090FF);
    draw_text_centered(&window, &font, "SUMMIT",
                       sg_sx, sg_sy-5, 2, 2, 2, 2, 0x3B2200FF);

    // --- TELEPORTER CAGE ---
    // world pos: center (1000, 260), size 80x80
    int tc_sx = sx(1000);
    int tc_sy = sy(260);
    int tc_hw = 40, tc_hh = 40; // half sizes

    // glow aura (pulsing approximated by static outer rect)
    draw_rect_centered(&window, tc_sx, tc_sy, 92, 92, 0x00FFFF22);
    draw_rect_centered(&window, tc_sx, tc_sy, 86, 86, 0x00FFFF11);

    // cage bars: outer frame
    draw_rect_centered(&window, tc_sx, tc_sy, 80, 4,  0x88EEFFFF); // top bar
    draw_rect_centered(&window, tc_sx, tc_sy+tc_hh-2, 80, 4, 0x88EEFFFF); // bottom bar (visual only, platform is below)
    draw_rect_centered(&window, tc_sx-tc_hw+2, tc_sy, 4, 80, 0x88EEFFFF); // left bar
    draw_rect_centered(&window, tc_sx+tc_hw-2, tc_sy, 4, 80, 0x88EEFFFF); // right bar
    // inner vertical bars
    draw_rect_centered(&window, tc_sx-14, tc_sy, 3, 76, 0x66CCDDFF);
    draw_rect_centered(&window, tc_sx,    tc_sy, 3, 76, 0x66CCDDFF);
    draw_rect_centered(&window, tc_sx+14, tc_sy, 3, 76, 0x66CCDDFF);

    // portal interior: cyan glow fill
    draw_rect_centered(&window, tc_sx, tc_sy, 68, 68, 0x00FFFF18);
    draw_rect_centered(&window, tc_sx, tc_sy, 44, 44, 0x00FFFF30);
    draw_rect_centered(&window, tc_sx, tc_sy, 20, 20, 0x00FFFFFF);

    // label above cage
    draw_text_centered(&window, &font, "ENTER",
                       tc_sx, tc_sy - tc_hh - 20, 2, 2, 2, 2, 0x00FFDDFF);
}

//###############################################
// HUD
//###############################################
void draw_hud(){
    char buf[128];

    // coins top-left
    sprintf(buf, "COINS %d", player.score);
    draw_rect(&window, 8, 8, 130, 24, 0x00000088);
    draw_text(&window, &font, buf, 12, 12, 2, 2, 2, 2, 0xFFD700FF);

    // height top-center
    double h_m = (WORLD_H - player.base.y) / 140.0;
    if(h_m < 0) h_m = 0;
    if(h_m > 100) h_m = 100;
    sprintf(buf, "%dm", (int)h_m);
    draw_rect(&window, SCREEN_W/2-70, 8, 140, 36, 0x00000088);
    draw_text_centered(&window, &font, buf,
                       SCREEN_W/2, 12, 2, 2, 2, 2, 0xFFFFFFFF);

    // progress bar
    int bw=160, bh=8, bx=SCREEN_W/2-bw/2, by=30;
    draw_rect(&window, bx, by, bw, bh, 0x333333FF);
    draw_rect(&window, bx, by, (int)(bw*h_m/100.0), bh, 0x00FF88FF);
    draw_rect(&window, bx, by, bw, 2, 0xFFFFFF44);

    // boost indicator
    if(player.jump_boost_timer > 0){
        draw_rect(&window, SCREEN_W/2-50, 46, 100, 20, 0xFFAA0099);
        draw_text_centered(&window, &font, "BOOST",
                           SCREEN_W/2, 49, 2, 2, 2, 2, 0xFFFFFFFF);
    }

    // --- LOCATION INDICATOR (single line, bottom of screen) ---
    // format:  W--[N]--E   X:1234  Y:5678  (compass + coords)
    int wx_pos = (int)player.base.x;
    int wy_pos = (int)player.base.y;
    // compass: derive facing from last_move_dir and rough X position
    // horizontal: W=left(x<500) E=right(x>1500) else center
    // vertical:   N=near top(y<3000) S=near bottom(y>11000) else mid
    const char* h_dir = (wx_pos < 600)  ? "W" :
                        (wx_pos > 1400) ? "E" : "-";
    const char* v_dir = (wy_pos < 3000)  ? "N" :
                        (wy_pos > 11000) ? "S" : "-";
    sprintf(buf, "%s%s  X%d  Y%d", v_dir, h_dir, wx_pos, wy_pos);
    int loc_w = 260, loc_h = 20;
    int loc_x = SCREEN_W/2 - loc_w/2;
    int loc_y = SCREEN_H - loc_h - 6;
    draw_rect(&window, loc_x, loc_y, loc_w, loc_h, 0x00000099);
    draw_text_centered(&window, &font, buf,
                       SCREEN_W/2, loc_y+3, 1, 1, 2, 1, 0xAADDFFFF);

    // god mode indicator
    if(player.god_mode){
        draw_rect(&window, SCREEN_W-110, 8, 100, 24, 0xFF880099);
        draw_text(&window, &font, "GOD", SCREEN_W-104, 12, 2, 2, 2, 2, 0xFFFFFFFF);
    }
        draw_rect(&window, 8, 36, 80, 20, 0x00CED188);
        draw_text(&window, &font, "GRAB", 12, 40, 2, 2, 2, 2, 0xFFFFFFFF);
}

//###############################################
// MENU
//###############################################
static void draw_btn(int x, int y, int w, int h,
                     const char* lbl, unsigned int col){
    // check hover
    int hov = (mouse.x>=x && mouse.x<=x+w && mouse.y>=y && mouse.y<=y+h);
    unsigned int bc = hov ? (col | 0x404040FF) : col;
    draw_rect(&window, x, y, w, h, bc);
    draw_rect(&window, x, y, w, 2, 0xFFFFFF55);
    draw_rect(&window, x, y+h-2, w, 2, 0x00000055);
    draw_text_centered(&window, &font, lbl,
                       x+w/2, y+h/2-6, 2, 2, 2, 2, 0xFFFFFFFF);
}

void draw_menu(){
    draw_rect(&window, 0, 0, SCREEN_W, SCREEN_H, 0x0A0F1AFF);

    // snow effect (static dots)
    for(int i=0; i<40; i++){
        int sx2 = (i*97+13) % SCREEN_W;
        int sy2 = (i*137+41) % SCREEN_H;
        int sr  = 2 + (i%3);
        draw_rect_centered(&window, sx2, sy2, sr, sr, 0xFFFFFF44);
    }

    // mountain silhouette
    for(int row=0; row<25; row++){
        int rw=500-row*18; int rx=SCREEN_W/2-rw/2+row*9;
        int ry=SCREEN_H-350+row*13;
        if(ry>0&&ry<SCREEN_H&&rw>0)
            draw_rect(&window,rx,ry,rw,14,0x1E2A3AFF);
    }

    draw_text_centered(&window, &font, "VERTICAL",
                       SCREEN_W/2, 140, 5, 5, 3, 5, 0xC8DCF0FF);
    draw_text_centered(&window, &font, "CLIMBER",
                       SCREEN_W/2, 198, 5, 5, 3, 5, 0x88BBDDFF);
    draw_text_centered(&window, &font, "REACH THE SUMMIT",
                       SCREEN_W/2, 262, 1, 1, 2, 1, 0x7090B0FF);

    if(menu_sub_state == 0){
        draw_btn(500,320,200,45,"NEW GAME",0x1A4A2AFF);
        draw_btn(500,380,200,45,"OPTIONS", 0x1A2A4AFF);
        draw_btn(500,440,200,45,"ABOUT",   0x2A1A4AFF);
        draw_btn(500,500,200,45,"EXIT",    0x4A1A1AFF);
        draw_text_centered(&window, &font, "ENTER FOR NEW GAME",
                           SCREEN_W/2, 565, 1, 1, 2, 1, 0x445566FF);
    } else if(menu_sub_state == 1){
        draw_text_centered(&window, &font, "CONTROLS",
                           SCREEN_W/2, 310, 2, 2, 2, 2, 0xC8DCF0FF);
        int lx=380, ly=360, ls=18;
        draw_text(&window,&font,"A D      MOVE LEFT RIGHT",lx,ly,       1,1,2,1,0xFFFFFFFF);
        draw_text(&window,&font,"SPACE    JUMP",           lx,ly+ls,    1,1,2,1,0xFFFFFFFF);
        draw_text(&window,&font,"SPACE    WALL JUMP",      lx,ly+ls*2,  1,1,2,1,0xFFFFFFFF);
        draw_text(&window,&font,"SHIFT    DASH",           lx,ly+ls*3,  1,1,2,1,0xFFFFFFFF);
        draw_text(&window,&font,"A OR D   WALL GRAB",      lx,ly+ls*4,  1,1,2,1,0xFFFFFFFF);
        draw_text(&window,&font,"STOMP    STUN ENEMIES",   lx,ly+ls*5,  1,1,2,1,0xFFFFFFFF);
        draw_text(&window,&font,"ESC      PAUSE OR BACK",  lx,ly+ls*7,  1,1,2,1,0x8899AAFF);
    } else if(menu_sub_state == 2){
        draw_text_centered(&window, &font, "ABOUT",
                           SCREEN_W/2, 310, 2, 2, 2, 2, 0xC8DCF0FF);
        int lx=380, ly=360, ls=20;
        draw_text(&window,&font,"MADE WITH SDL2 AND C",  lx,ly,      1,1,2,1,0xFFFFFFFF);
        draw_text(&window,&font,"CLIMB 100M TO WIN",     lx,ly+ls,   1,1,2,1,0xFFFFFFFF);
        draw_text(&window,&font,"STOMP ENEMIES",         lx,ly+ls*2, 1,1,2,1,0xFFFFFFFF);
        draw_text(&window,&font,"BREAK CRACKED ROCKS",   lx,ly+ls*3, 1,1,2,1,0xFFFFFFFF);
        draw_text(&window,&font,"COLLECT GOLD COINS",    lx,ly+ls*4, 1,1,2,1,0xFFFFFFFF);
        draw_text(&window,&font,"GRAB STAR FOR BOOST",   lx,ly+ls*5, 1,1,2,1,0xFFFFFFFF);
        draw_text(&window,&font,"ESC  BACK",             lx,ly+ls*7, 1,1,2,1,0x8899AAFF);
    }
}

//###############################################
// WIN SCREEN
//###############################################
void draw_win_screen(){
    draw_rect(&window, 0, 0, SCREEN_W, SCREEN_H, 0x00000099);
    draw_text_centered(&window, &font, "SUMMIT REACHED",
                       SCREEN_W/2, 220, 4, 4, 3, 4, 0xFFFF88FF);
    char buf[64];
    sprintf(buf,"COINS %d", player.score);
    draw_text_centered(&window, &font, buf,
                       SCREEN_W/2, 330, 2, 2, 2, 2, 0xFFD700FF);
    draw_btn(500,450,200,50,"PLAY AGAIN",0x1A4A2AFF);
    draw_text_centered(&window, &font, "SPACE TO REPLAY",
                       SCREEN_W/2, 530, 1, 1, 2, 1, 0x667788FF);
}

//###############################################
// PAUSE SCREEN
//###############################################
void draw_pause_screen(){
    draw_rect(&window, 0, 0, SCREEN_W, SCREEN_H, 0x00000099);
    draw_text_centered(&window, &font, "PAUSED",
                       SCREEN_W/2, SCREEN_H/2-20, 4, 4, 2, 4, 0xFFFFFFFF);
    draw_text_centered(&window, &font, "ESC TO RESUME",
                       SCREEN_W/2, SCREEN_H/2+40, 1, 1, 2, 1, 0x8899AAFF);
}

#endif
