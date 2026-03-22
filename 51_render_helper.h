#ifndef RENDER_HELPER_H
#define RENDER_HELPER_H
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "10_data.h"
#include "external_lib/physic.h"

// world meters -> screen pixels
#define SX(wx) m2sx((wx), camera.x_px, cfg.px_per_m)
#define SY(wy) m2sy((wy), camera.y_px, cfg.px_per_m)
#define SP(sm) m2sp((sm), cfg.px_per_m)

//###############################################
// BACKGROUND
//###############################################
static unsigned int lerp_color(unsigned int c0,unsigned int c1,double t){
    if(t<0)t=0; if(t>1)t=1;
    int r0=(c0>>24)&0xFF,g0=(c0>>16)&0xFF,b0=(c0>>8)&0xFF;
    int r1=(c1>>24)&0xFF,g1=(c1>>16)&0xFF,b1=(c1>>8)&0xFF;
    int r=(int)(r0+(r1-r0)*t),g=(int)(g0+(g1-g0)*t),b=(int)(b0+(b1-b0)*t);
    return ((unsigned int)r<<24)|((unsigned int)g<<16)|((unsigned int)b<<8)|0xFF;
}

void draw_background(){
    // height in meters from base (0=base, world_h=summit)
    double h_m = cfg.world_h - player.base.y;
    unsigned int c_base   = 0x1A1214FF;
    unsigned int c_lower  = 0x2D2B3DFF;
    unsigned int c_mid    = 0x3D5A80FF;
    unsigned int c_upper  = 0x7090B0FF;
    unsigned int c_summit = 0xC8DCF0FF;
    unsigned int bg;
    if     (h_m<15){ double t=h_m/15.0;         bg=lerp_color(c_lower,c_base,1.0-t); }
    else if(h_m<40){ double t=(h_m-15.0)/25.0;  bg=lerp_color(c_lower,c_mid,t); }
    else if(h_m<65){ double t=(h_m-40.0)/25.0;  bg=lerp_color(c_mid,c_upper,t); }
    else if(h_m<85){ double t=(h_m-65.0)/20.0;  bg=lerp_color(c_upper,c_summit,t); }
    else             bg=c_summit;
    draw_rect(&window,0,0,(int)cfg.screen_w,(int)cfg.screen_h,bg);

    if(h_m<40){
        int sw=(int)cfg.screen_w, sh=(int)cfg.screen_h;
        double para=0.3;
        int mx=(int)(camera.x_px*para);
        unsigned int mc=lerp_color(0x444450FF,bg,0.5);
        for(int row=0;row<20;row++){
            int rw=sw/4-row*14, rx=sw/12-mx%sw-rw/2+row*7;
            int ry=sh-sh*38/100+row*13;
            if(ry>0&&ry<sh) draw_rect(&window,rx,ry,rw>0?rw:0,14,mc);
        }
        for(int row=0;row<20;row++){
            int rw=sw*23/100-row*13, rx=sw*67/100-mx%sw-rw/2+row*6;
            int ry=sh-sh*35/100+row*12;
            if(ry>0&&ry<sh) draw_rect(&window,rx,ry,rw>0?rw:0,13,mc);
        }
    }
}

//###############################################
// TERRAIN
//###############################################
void draw_terrain(){
    for(int i=0;i<terrain_count_actual;i++){
        if(terrains[i].broken) continue;
        double tx=terrains[i].base.x, ty=terrains[i].base.y;
        double tw=terrains[i].base.col_w, th=terrains[i].base.col_h;
        int dsx=SX(tx-tw/2), dsy=SY(ty-th/2);
        int dsw=SP(tw),       dsh=SP(th);
        if(dsy+dsh<0||dsy>(int)cfg.screen_h) continue;
        if(dsx+dsw<0||dsx>(int)cfg.screen_w) continue;

        if(terrains[i].type==TERRAIN_BREAK){
            draw_rect(&window,dsx,dsy,dsw,dsh,0x9B6B4BFF);
            draw_line(&window,dsx+4,dsy+4,dsx+dsw-4,dsy+dsh-4,0x5A3A2AFF);
            draw_line(&window,dsx+dsw-4,dsy+4,dsx+4,dsy+dsh-4,0x5A3A2AFF);
        } else {
            double t_h_m = cfg.world_h - ty;  // meters from base
            unsigned int rock_col;
            if     (t_h_m>70) rock_col=0xDCDCECFF;
            else if(t_h_m>40) rock_col=0x8A8A9AFF;
            else               rock_col=0x5A5A6AFF;
            draw_rect(&window,dsx,dsy,dsw,dsh,rock_col);
            unsigned int top_col=(t_h_m>70)?0xFFFFFFFF:(t_h_m>40)?0xBBBBCCFF:0x777788FF;
            draw_rect(&window,dsx,dsy,dsw,3,top_col);
            draw_rect(&window,dsx,dsy+dsh-3,dsw,3,0x00000033);
        }
        
        if(terrains[i].warning_timer > 0 && ((int)(tps_timer.time * 8) % 2 == 0)) {
            draw_rect(&window, dsx, dsy, dsw, dsh, 0xFF000077);
        }
    }
}

//###############################################
// PLAYER
//###############################################
void draw_player(){
    int spx=SX(player.base.x), spy=SY(player.base.y);
    int pw=SP(cfg.player_w),    ph=SP(cfg.player_h);
    int head_w=SP(0.20), head_h=SP(0.18);
    int eye_s=SP(0.05);
    if(player.invincible>0&&(player.invincible/3)%2==0) return;
    unsigned int body=0x4169E1FF;
    if(player.edge_grab)          body=0x00CED1FF;
    if(player.jump_boost_timer>0) body=0xFFAA00FF;//vàng
    else if(player.speed_boost_timer > 0) body=0x00FFCCFF;//xanh ngọc

    draw_rect_centered(&window,spx,spy,pw,ph,body);
    draw_rect_centered(&window,spx,spy-ph/2-head_h/2,head_w,head_h,0xFFD700FF);
    int ex=(player.last_move_dir>0)?spx+eye_s:spx-eye_s;
    draw_rect_centered(&window,ex,spy-ph/2-head_h/2-eye_s/2,eye_s,eye_s,0x222222FF);
    if(player.dashing>0){
        for(int t=1;t<=3;t++){
            int tx2=spx-player.dash_dir*t*SP(0.14);
            draw_rect_centered(&window,tx2,spy,pw-t*SP(0.05),ph-t*SP(0.05),0xFFFFFF22);
        }
    }
    if(player.edge_grab){
        int hx=spx+player.grab_wall_dir*(pw/2+SP(0.03));
        int hs=SP(0.06);
        draw_rect_centered(&window,hx,spy-hs,hs,SP(0.05),0xFFD700FF);
        draw_rect_centered(&window,hx,spy+hs,hs,SP(0.05),0xFFD700FF);
    }
}


void draw_player_sensors(){
    // inactive color (dim, semi-transparent)
    unsigned int c_off = 0xFFFFFF33;
    // active colors per direction
    unsigned int c_d = player.sensors.d_active ? 0x00FF00FF : c_off;  // down  = green
    unsigned int c_u = player.sensors.u_active ? 0xFF4444FF : c_off;  // up    = red
    unsigned int c_l = player.sensors.l_active ? 0x4488FFFF : c_off;  // left  = blue
    unsigned int c_r = player.sensors.r_active ? 0xFFAA00FF : c_off;  // right = orange

    // convert sensor world coords to screen pixels and draw
    draw_rect_centered(&window,
        m2sx(player.sensors.dx, camera.x_px, cfg.px_per_m),
        m2sy(player.sensors.dy, camera.y_px, cfg.px_per_m),
        m2sp(player.sensors.dw, cfg.px_per_m),
        m2sp(player.sensors.dh, cfg.px_per_m), c_d);

    draw_rect_centered(&window,
        m2sx(player.sensors.ux, camera.x_px, cfg.px_per_m),
        m2sy(player.sensors.uy, camera.y_px, cfg.px_per_m),
        m2sp(player.sensors.uw, cfg.px_per_m),
        m2sp(player.sensors.uh, cfg.px_per_m), c_u);

    draw_rect_centered(&window,
        m2sx(player.sensors.lx, camera.x_px, cfg.px_per_m),
        m2sy(player.sensors.ly, camera.y_px, cfg.px_per_m),
        m2sp(player.sensors.lw, cfg.px_per_m),
        m2sp(player.sensors.lh, cfg.px_per_m), c_l);

    draw_rect_centered(&window,
        m2sx(player.sensors.rx, camera.x_px, cfg.px_per_m),
        m2sy(player.sensors.ry, camera.y_px, cfg.px_per_m),
        m2sp(player.sensors.rw, cfg.px_per_m),
        m2sp(player.sensors.rh, cfg.px_per_m), c_r);
}


//###############################################
// ENEMIES
//###############################################
void draw_enemies(){
    static double last_time = -1.0;
    double current_time = tps_timer.time;
    double dt_render = 0.0;
    if(last_time != current_time) {
        dt_render = (last_time < 0) ? 0.0 : (current_time - last_time);
        if(dt_render > 0.1) dt_render = 0.1;
        last_time = current_time;
    }
    
    static double leaf_ox[ENEMY_COUNT] = {0};
    static double leaf_oy[ENEMY_COUNT] = {0};

    for(int i=0;i<enemy_count_actual;i++){
        struct enemy_data* e=&enemies[i];
        int esx=SX(e->base.x), esy=SY(e->base.y);

        int cull_margin = (e->type == ENEMY_WEATHER_BOSS) ? 1800 : 100;
        if(esy < -cull_margin || esy > (int)cfg.screen_h + cull_margin) continue;

        int ew=SP(e->base.col_w), eh=SP(e->base.col_h);

        if(!e->active){
            draw_rect_centered(&window,esx,esy,ew,eh,0x666666FF);
            draw_line(&window,esx-ew/5,esy-eh/5,esx+ew/5,esy+eh/5,0x222222FF);
            draw_line(&window,esx+ew/5,esy-eh/5,esx-ew/5,esy+eh/5,0x222222FF);
            continue;
        }
        unsigned int col;
        if(e->type == ENEMY_BOSS) col=0xFF2200FF;
        else if(e->type == ENEMY_SHOOTER) col=0x32CD32FF; 
        else if(e->type == ENEMY_SWORD) col=0xA9A9A9FF; 
        else if(e->type == ENEMY_MAGE) col=0x8A2BE2FF;
        else if(e->type == ENEMY_WEATHER_BOSS) col=0x00BFFFFF;
        else col=0xCC3333FF; 

        if(e->dashing) col=0xFF7700FF;
        draw_rect_centered(&window,esx,esy,ew,eh,col);
        int eye_s=SP(0.07);
        draw_rect_centered(&window,esx-ew/5,esy-eh/5,eye_s,eye_s,0xFFFF00FF);
        draw_rect_centered(&window,esx+ew/5,esy-eh/5,eye_s,eye_s,0xFFFF00FF);

        if(e->hp > 0){
            int max_hp = 1;
            if(e->type == ENEMY_BOSS) max_hp = 5;
            else if(e->type == ENEMY_WEATHER_BOSS) max_hp = 4;
            else if(e->type == ENEMY_SWORD || e->type == ENEMY_MAGE) max_hp = 2;
            
            int bw, bh, bx, by;
            if(e->type == ENEMY_BOSS || e->type == ENEMY_WEATHER_BOSS || e->type == ENEMY_MAGE){
                bw=SP(0.8); bh=SP(0.1); bx=esx-SP(0.4); by=esy-eh/2-SP(0.16);
            }
            else{
                bw=SP(0.5); bh=SP(0.05); bx=esx-SP(0.25); by=esy-eh/2-SP(0.16);
            }
            
            draw_rect(&window, bx, by, bw, bh, 0x333333FF);
            draw_rect(&window, bx, by, bw * e->hp / max_hp, bh, 0xFF3333F0);
        }

        if(e->type == ENEMY_WEATHER_BOSS && e->hp > 0) {
            
            leaf_ox[i] += (e->dash_vx * 0.6) * dt_render; 
            leaf_oy[i] += 2.0 * dt_render;                
            
            double boss_x = e->base.x;
            double boss_y = e->base.y;

            for(int k = 0; k < 120; k++) { 
                
                double noise_x = fmod(fabs(sin(k * 99.123) * 12345.67), 36.0); 
                double noise_y = fmod(fabs(cos(k * 88.321) * 76543.21), 36.0); 
                
                double move_x = noise_x + leaf_ox[i]; 
                double move_y = noise_y + leaf_oy[i]; 
                
                move_x = fmod(move_x, 36.0);
                if(move_x < 0) move_x += 36.0;
                move_x -= 18.0;
                
                move_y = fmod(move_y, 36.0);
                if(move_y < 0) move_y += 36.0;
                move_y -= 18.0;
                
                if(move_x*move_x + move_y*move_y < 324.0) {
                    int psx = SX(boss_x + move_x);
                    int psy = SY(boss_y + move_y);
                    
                    unsigned int leaf_col;
                    if(k % 3 == 0)      leaf_col = 0x228B22FF; 
                    else if(k % 3 == 1) leaf_col = 0x32CD32FF; 
                    else                leaf_col = 0x8B4513FF; 
                    
                    if(k % 2 == 0) draw_rect(&window, psx, psy, 10, 6, leaf_col); 
                    else           draw_rect(&window, psx, psy, 6, 10, leaf_col); 
                }
            }
        }
    }
}

//###############################################
// COINS
//###############################################
void draw_coins(){
    for(int i=0;i<coin_count_actual;i++){
        if(coins[i].collected) continue;
        int csx=SX(coins[i].x), csy=SY(coins[i].y);
        if(csy<-20||csy>(int)cfg.screen_h+20) continue;
        int cs=SP(0.16), ci=SP(0.09);
        draw_rect_centered(&window,csx,csy,cs,cs,0xFFD700FF);
        draw_rect_centered(&window,csx,csy,ci,ci,0xFFF5B0FF);
    }
}

//###############################################
// ITEMS
//###############################################
void draw_items(){
    for(int i=0;i<item_count_actual;i++){
        if(!items[i].active) continue;
        int isx=SX(items[i].x), isy=SY(items[i].y);
        if(isy<-20||isy>(int)cfg.screen_h+20) continue;
        int ia=SP(0.22), ib=SP(0.08), ic2=SP(0.14), id=SP(0.06);
        if(items[i].type == 1){
            draw_rect_centered(&window,isx,isy,ia,ib,0x00FFFFFF);
            draw_rect_centered(&window,isx,isy,ib,ia,0x00FFFFFF);
            draw_rect_centered(&window,isx,isy,ic2,ic2,0x00AAFFFF);
        }
        else if(items[i].type == 2){
            draw_rect_centered(&window,isx,isy,ia,ib,0xFFFF00FF);
            draw_rect_centered(&window,isx,isy,ib,ia,0xFFFF00FF);
            draw_rect_centered(&window,isx,isy,ic2,ic2,0xFFAA00FF);
        }
        else if(items[i].type == 3){
            draw_rect_centered(&window,isx,isy,SP(0.22),SP(0.22),0xCC1100FF); 
            draw_rect_centered(&window,isx,isy,SP(0.16),SP(0.16),0xFF4400FF); 
            draw_rect_centered(&window,isx,isy,SP(0.08),SP(0.08),0xFFFF00FF); 
        }
        
        draw_rect_centered(&window,isx,isy,id,id,0xFFFFFFFF);
    }
}

//###############################################
// DECOR
//###############################################
void draw_decors(){
    for(int i=0;i<decor_count_actual;i++){
        struct decor_data* d=&decors[i];
        int dx=SX(d->x), dy=SY(d->y);
        int dw=SP(d->w), dh=SP(d->h);

        // cull
        if(dy+dh<0||dy>(int)cfg.screen_h) continue;
        if(dx+dw<0||dx>(int)cfg.screen_w) continue;

        if(d->is_teleporter){
            // animated cage: layered cyan rects + bars
            draw_rect_centered(&window,dx,dy,dw+SP(0.12),dh+SP(0.12),0x00FFFF22);
            draw_rect_centered(&window,dx,dy,dw+SP(0.06),dh+SP(0.06),0x00FFFF11);
            // frame bars
            draw_rect_centered(&window,dx,      dy,            dw,SP(0.04),0x88EEFFFF);
            draw_rect_centered(&window,dx,      dy+dh/2-SP(0.02),dw,SP(0.04),0x88EEFFFF);
            draw_rect_centered(&window,dx-dw/2+SP(0.02),dy,  SP(0.04),dh,0x88EEFFFF);
            draw_rect_centered(&window,dx+dw/2-SP(0.02),dy,  SP(0.04),dh,0x88EEFFFF);
            // inner vertical bars
            draw_rect_centered(&window,dx-dw/5,dy,SP(0.03),dh-SP(0.04),0x66CCDDFF);
            draw_rect_centered(&window,dx,     dy,SP(0.03),dh-SP(0.04),0x66CCDDFF);
            draw_rect_centered(&window,dx+dw/5,dy,SP(0.03),dh-SP(0.04),0x66CCDDFF);
            // portal glow interior
            draw_rect_centered(&window,dx,dy,dw/2,dh/2,0x00FFFF30);
            draw_rect_centered(&window,dx,dy,dw/4,dh/4,0x00FFFFFF);
            draw_text_centered(&window,&font,"ENTER",
                               dx,dy-dh/2-SP(0.15),2,2,2,2,0x00FFDDFF);
        } else {
            // plain rect
            draw_rect_centered(&window,dx,dy,dw,dh,d->color);
            // label if present
            if(d->label[0]!='\0'){
                draw_text_centered(&window,&font,d->label,
                                   dx,dy-4,2,2,2,2,0x3B2200FF);
            }
        }
    }
}

//###############################################
// POBJS  (pushable physics objects)
//###############################################
void draw_pobjs(){
    for(int i=0;i<pobj_count_actual;i++){
        struct pobj_data* p=&pobjs[i];
        if(!p->active) continue;
        int px=SX(p->base.x), py=SY(p->base.y);
        int pw=SP(p->base.col_w), ph=SP(p->base.col_h);
        if(py+ph<0||py>(int)cfg.screen_h) continue;
        if(px+pw<0||px>(int)cfg.screen_w) continue;
        draw_rect_centered(&window,px,py,pw,ph,p->color);
        // top highlight + bottom shadow for depth
        draw_rect_centered(&window,px,py-ph/2+1,pw,2,0xFFFFFF44);
        draw_rect_centered(&window,px,py+ph/2-1,pw,2,0x00000044);
    }
}

//###############################################
// FIREBALL
//###############################################
void draw_projectiles(){
    for(int i = 0; i < PROJ_COUNT; i++){
        if(!projectiles[i].active) continue;
        int px = SX(projectiles[i].x), py = SY(projectiles[i].y);
        
        if(projectiles[i].type == 0) {
            draw_rect_centered(&window, px, py, SP(0.2), SP(0.2), 0xFF4500FF);
            draw_rect_centered(&window, px, py, SP(0.1), SP(0.1), 0xFFD700FF);
        } 
        else {
            draw_rect_centered(&window, px, py, SP(0.2), SP(0.2), 0x8A2BE2FF); 
            draw_rect_centered(&window, px, py, SP(0.1), SP(0.1), 0x00FF00FF); 
        }
    }
}

//###############################################
// HUD  (exact copy from user, duplicate GRAB removed)
//###############################################
void draw_hud(){
    char buf[128];
    int sw=(int)cfg.screen_w, sh=(int)cfg.screen_h;
    int cx=sw/2;
    int pad=sw*7/1000;    // ~8px at 1200
    int row_h=sh*3/100;   // ~24px at 800
    int bar_w=sw*11/100;  // ~130px at 1200

    // --- top-left stats ---
    draw_rect(&window, pad, pad,           bar_w, row_h, 0x00000088);
    sprintf(buf, "COINS %d", player.score);
    draw_text(&window, &font, buf, pad+4, pad+4, 2,2,2,2, 0xFFD700FF);

    draw_rect(&window, pad, pad+row_h+4,   bar_w, row_h, 0x00000088);
    sprintf(buf, "HP: %d", player.hp);
    draw_text(&window, &font, buf, pad+4, pad+row_h+8, 2,2,2,2, 0xFF3333FF);

    draw_rect(&window, pad, pad+row_h*2+8, bar_w, row_h, 0x00000088);
    sprintf(buf, "FIRE: %d", player.fireball_ammo);
    draw_text(&window, &font, buf, pad+4, pad+row_h*2+12, 2,2,2,2, 0xFF6600FF);

    // --- top-center block ---
    double h_m = cfg.world_h - player.base.y;
    if(h_m<0) h_m=0;
    if(h_m>cfg.world_h) h_m=cfg.world_h;
    double x_m = player.base.x;
    const char* ns = (h_m > cfg.world_h*0.70)?"N":(h_m < cfg.world_h*0.20)?"S":"-";
    const char* ew = (x_m < cfg.world_w*0.20)?"W":(x_m > cfg.world_w*0.70)?"E":"-";
    sprintf(buf, "%s%s  X%.1fm  Y%.1fm", ns, ew, x_m, h_m);
    int blk_w=sw*22/100, blk_h=sh*58/1000;  // ~260x46 at 1200x800
    int blk_x=cx-blk_w/2;
    draw_rect(&window, blk_x, 6, blk_w, blk_h, 0x00000099);
    draw_text_centered(&window, &font, buf, cx, 10, 1,1,2,1, 0xAADDFFFF);
    sprintf(buf, "%.0fm", h_m);
    draw_text_centered(&window, &font, buf, cx, 24, 2,2,2,2, 0xFFFFFFFF);
    int pbw=sw*13/100, pbh=sh*9/1000, pbx=cx-pbw/2, pby=40;  // progress bar
    draw_rect(&window, pbx, pby, pbw, pbh, 0x333333FF);
    draw_rect(&window, pbx, pby, (int)(pbw*h_m/cfg.world_h), pbh, 0x00FF88FF);
    draw_rect(&window, pbx, pby, pbw, 2, 0xFFFFFF33);

    // --- boost badges ---
    int hud_y=blk_h+10;
    if(player.jump_boost_timer>0){
        draw_rect(&window, cx-sw*7/100, hud_y, sw*6/100, sh*23/1000, 0xFFAA0099);
        draw_text_centered(&window, &font, "JUMP", cx-sw*4/100, hud_y+sh*14/1000, 1,1,2,1, 0xFFFFFFFF);
    }
    if(player.speed_boost_timer>0){
        draw_rect(&window, cx+sw*1/100, hud_y, sw*6/100, sh*23/1000, 0x00AAFF99);
        draw_text_centered(&window, &font, "SPEED", cx+sw*4/100, hud_y+sh*14/1000, 1,1,2,1, 0xFFFFFFFF);
    }

    // --- top-right badges ---
    if(player.edge_grab){
        draw_rect(&window, sw-sw*6/100, sh*25/1000, sw*5/100, sh*25/1000, 0x00CED188);
        draw_text(&window, &font, "GRAB", sw-sw*45/1000, sh*32/1000, 1.2,1.2,2,1, 0xFFFFFFFF);
    }
    if(player.god_mode){
        draw_rect(&window, sw-sw*92/1000, pad, sw*83/1000, row_h, 0xFF880099);
        draw_text(&window, &font, "GOD", sw-sw*87/1000, pad+4, 2,2,2,2, 0xFFFFFFFF);
    }
}

//###############################################
// MENU
//###############################################
static void draw_btn(int x,int y,int w,int h,const char* lbl,unsigned int col){
    int hov=(mouse.x>=x&&mouse.x<=x+w&&mouse.y>=y&&mouse.y<=y+h);
    draw_rect(&window,x,y,w,h,hov?(col|0x404040FF):col);
    draw_rect(&window,x,y,w,2,0xFFFFFF55);
    draw_rect(&window,x,y+h-2,w,2,0x00000055);
    draw_text_centered(&window,&font,lbl,x+w/2,y+h/2-6,2,2,2,2,0xFFFFFFFF);
}

void draw_menu(){
    int sw=(int)cfg.screen_w, sh=(int)cfg.screen_h;
    int cx=sw/2;
    draw_rect(&window,0,0,sw,sh,0x0A0F1AFF);
    for(int i=0;i<40;i++){
        int sx2=(i*97+13)%sw, sy2=(i*137+41)%sh, sr=2+(i%3);
        draw_rect_centered(&window,sx2,sy2,sr,sr,0xFFFFFF44);
    }
    for(int row=0;row<25;row++){
        int rw=sw*5/12-row*18, rx=cx-rw/2+row*9, ry=sh-sh*44/100+row*13;
        if(ry>0&&ry<sh&&rw>0) draw_rect(&window,rx,ry,rw,14,0x1E2A3AFF);
    }
    draw_text_centered(&window,&font,"VERTICAL",cx,sh*18/100,5,5,3,5,0xC8DCF0FF);
    draw_text_centered(&window,&font,"CLIMBER", cx,sh*25/100,5,5,3,5,0x88BBDDFF);
    draw_text_centered(&window,&font,"REACH THE SUMMIT",cx,sh*33/100,1,1,2,1,0x7090B0FF);
    int bw=sw/6, bh=sh*6/100, bx=cx-bw/2;
    if(menu_sub_state==0){
        draw_btn(bx,sh*40/100,bw,bh,"NEW GAME",0x1A4A2AFF);
        draw_btn(bx,sh*48/100,bw,bh,"OPTIONS", 0x1A2A4AFF);
        draw_btn(bx,sh*56/100,bw,bh,"ABOUT",   0x2A1A4AFF);
        draw_btn(bx,sh*64/100,bw,bh,"EXIT",    0x4A1A1AFF);
        draw_text_centered(&window,&font,"ENTER FOR NEW GAME",cx,sh*72/100,1,1,2,1,0x445566FF);
    } else if(menu_sub_state==1){
        draw_text_centered(&window,&font,"CONTROLS",cx,sh*39/100,2,2,2,2,0xC8DCF0FF);
        int lx=cx-sw/6, ly=sh*45/100, ls=sh*23/1000;
        draw_text(&window,&font,"A D      MOVE",         lx,ly,       1,1,2,1,0xFFFFFFFF);
        draw_text(&window,&font,"SPACE    JUMP",         lx,ly+ls,    1,1,2,1,0xFFFFFFFF);
        draw_text(&window,&font,"SHIFT    DASH",         lx,ly+ls*2,  1,1,2,1,0xFFFFFFFF);
        draw_text(&window,&font,"A OR D   WALL GRAB",   lx,ly+ls*3,  1,1,2,1,0xFFFFFFFF);
        draw_text(&window,&font,"G        GOD MODE",    lx,ly+ls*4,  1,1,2,1,0xFFFFFFFF);
        draw_text(&window,&font,"K        RELOAD WORLD",lx,ly+ls*5,  1,1,2,1,0xFFFFFFFF);
        draw_text(&window,&font,"ESC      PAUSE",       lx,ly+ls*6,  1,1,2,1,0xFFFFFFFF);
        draw_text(&window,&font,"ESC      BACK",        lx,ly+ls*8,  1,1,2,1,0x8899AAFF);
    } else {
        draw_text_centered(&window,&font,"ABOUT",cx,sh*39/100,2,2,2,2,0xC8DCF0FF);
        int lx=cx-sw/6, ly=sh*45/100, ls=sh*25/1000;
        draw_text(&window,&font,"MADE WITH SDL2 AND C",  lx,ly,      1,1,2,1,0xFFFFFFFF);
        draw_text(&window,&font,"CLIMB 100M TO WIN",     lx,ly+ls,   1,1,2,1,0xFFFFFFFF);
        draw_text(&window,&font,"STOMP ENEMIES",         lx,ly+ls*2, 1,1,2,1,0xFFFFFFFF);
        draw_text(&window,&font,"BREAK CRACKED ROCKS",  lx,ly+ls*3, 1,1,2,1,0xFFFFFFFF);
        draw_text(&window,&font,"COLLECT GOLD COINS",   lx,ly+ls*4, 1,1,2,1,0xFFFFFFFF);
        draw_text(&window,&font,"GET STAR FOR BOOST",   lx,ly+ls*5, 1,1,2,1,0xFFFFFFFF);
        draw_text(&window,&font,"ESC  BACK",            lx,ly+ls*7, 1,1,2,1,0x8899AAFF);
    }
}

//###############################################
// CHEST
//###############################################
void draw_chests(){
    for(int i=0;i<chest_count_actual;i++){
        int cx = SX(chests[i].x), cy = SY(chests[i].y);
        int cw = SP(0.6), ch = SP(0.4);

        if(chests[i].state == 0){ // Đang đóng
            draw_rect_centered(&window, cx, cy, cw, ch, 0x8B4513FF);
            draw_rect_centered(&window, cx, cy, cw+2, 4, 0x000000FF);
            draw_rect_centered(&window, cx, cy, 6, 8, 0xFFD700FF); // Ổ khóa
        } else { // Mở tung
            draw_rect_centered(&window, cx, cy + ch/4, cw, ch/2, 0x8B4513FF);
            draw_rect_centered(&window, cx, cy - ch/4, cw, ch/4, 0x6B3A2AFF); 
        }

        if(chests[i].show_key){
            draw_rect_centered(&window, cx, cy - SP(0.6), SP(0.3), SP(0.3), 0x00000088);
            draw_text_centered(&window, &font, "E", cx, cy - SP(0.6) - 5, 2,2,2,2, 0xFFFFFFFF);
        }
    }
}

//###############################################
// WIN SCREEN
//###############################################
void draw_win_screen(){
    int sw=(int)cfg.screen_w, sh=(int)cfg.screen_h;
    int cx=sw/2;
    int bw=sw/6, bh=sh*6/100, bx=cx-bw/2;
    draw_rect(&window,0,0,sw,sh,0x00000099);
    draw_text_centered(&window,&font,"SUMMIT REACHED",cx,sh*28/100,4,4,3,4,0xFFFF88FF);
    char buf[64]; sprintf(buf,"COINS %d",player.score);
    draw_text_centered(&window,&font,buf,cx,sh*42/100,2,2,2,2,0xFFD700FF);
    draw_btn(bx,sh*50/100,bw,bh,"PLAY AGAIN",0x1A4A2AFF);
    draw_btn(bx,sh*58/100,bw,bh,"MAIN MENU", 0x2A2A4AFF);
    draw_text_centered(&window,&font,"SPACE TO REPLAY",cx,sh*68/100,1,1,2,1,0x667788FF);
}

//###############################################
// PAUSE SCREEN
//###############################################
void draw_pause_screen(){
    int sw=(int)cfg.screen_w, sh=(int)cfg.screen_h;
    int cx=sw/2;
    int bw=sw/6, bh=sh*6/100, bx=cx-bw/2;
    draw_rect(&window,0,0,sw,sh,0x00000099);
    draw_text_centered(&window,&font,"PAUSED",cx,sh*42/100,4,4,2,4,0xFFFFFFFF);
    draw_text_centered(&window,&font,"ESC TO RESUME",cx,sh*50/100,1,1,2,1,0x8899AAFF);
    draw_btn(bx,sh*56/100,bw,bh,"MAIN MENU",0x2A2A4AFF);
}

#endif
