#ifndef RENDER_HELPER_H
#define RENDER_HELPER_H
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "10_data.h"
#include "external_lib/physic.h"

//###############################################
// SCREEN CONVERSION MACROS
//###############################################
#define SX(wx)   m2sx((wx), camera.x_px, cfg.px_per_m)
#define SY(wy)   m2sy((wy), camera.y_px, cfg.px_per_m)
#define SP(sm)   m2sp((sm), cfg.px_per_m)
#define SW       ((int)cfg.screen_w)
#define SH       ((int)cfg.screen_h)

// Draw a rect centered at world pos (meters)
#define WRECT(wx,wy,ww,wh,col) \
    draw_rect_centered(&window, SX(wx), SY(wy), SP(ww), SP(wh), col)

// Off-screen cull check (top-left origin rect)
#define CULL(sx,sy,sw2,sh2) \
    ((sy)+(sh2)<0 || (sy)>SH || (sx)+(sw2)<0 || (sx)>SW)

//###############################################
// COLOR HELPERS
//###############################################
static unsigned int lerp_color(unsigned int c0, unsigned int c1, double t){
    if(t<0) t=0; if(t>1) t=1;
    int r0=(c0>>24)&0xFF, g0=(c0>>16)&0xFF, b0=(c0>>8)&0xFF;
    int r1=(c1>>24)&0xFF, g1=(c1>>16)&0xFF, b1=(c1>>8)&0xFF;
    return (unsigned int)((int)(r0+(r1-r0)*t)<<24 |
                          (int)(g0+(g1-g0)*t)<<16 |
                          (int)(b0+(b1-b0)*t)<<8  | 0xFF);
}

static unsigned int hash_u32(int x, int y){
    unsigned int h = (unsigned int)(x*374761393 + y*668265263);
    h = (h^(h>>13)) * 1274126177u;
    return h^(h>>16);
}
static double hash01(int x, int y){ return (double)(hash_u32(x,y)%10000)/10000.0; }

//###############################################
// BACKGROUND
//###############################################
void draw_background(){
    double h_m = cfg.world_h - player.base.y;

    static const unsigned int BANDS[]  = { 0x1A1214FF,0x2D2B3DFF,0x3D5A80FF,0x7090B0FF,0xC8DCF0FF };
    static const double       CUTS[]   = { 15.0, 40.0, 65.0, 85.0 };
    unsigned int bg = BANDS[4];
    for(int i=0;i<4;i++){
        if(h_m < CUTS[i]){
            double t = (i==0) ? h_m/CUTS[0] : (h_m-CUTS[i-1])/(CUTS[i]-CUTS[i-1]);
            bg = lerp_color(BANDS[i], BANDS[i+1], (i==0)?(1.0-t):t);
            break;
        }
    }
    draw_rect(&window, 0, 0, SW, SH, bg);

    // Tiled star-field dots (3 bands, coarse→fine top→bottom)
    double ht = h_m / cfg.world_h;
    static const int    TILE_SZ[3] = {192, 96, 48};
    static const double BAND_Y0[3] = {0.0, 0.33, 0.66};
    static const double BAND_Y1[3] = {0.33, 0.66, 1.0};

    for(int band=0; band<3; band++){
        int tile = TILE_SZ[band];
        int cols = SW/tile+2, rows = SH/tile+2;
        int ox=(int)camera.x_px/tile, oy=(int)camera.y_px/tile;
        int px_off=(int)camera.x_px%tile, py_off=(int)camera.y_px%tile;

        for(int y=0;y<rows;y++) for(int x=0;x<cols;x++){
            int sx=x*tile-px_off, sy=y*tile-py_off;
            double h_ratio=(cfg.world_h-(camera.y_px+sy)/cfg.px_per_m)/cfg.world_h;
            if(h_ratio<BAND_Y0[band]||h_ratio>=BAND_Y1[band]) continue;
            double t = ht*0.85 + hash01(x+ox,y+oy)*0.15;
            draw_rect(&window,sx,sy,tile+1,tile+1,lerp_color(0x303038FF,0xA0A0A8FF,t));
        }
    }
}

//###############################################
// TERRAIN
//###############################################
void draw_terrain(){
    for(int i=0;i<terrain_count_actual;i++){
        struct terrain_data* t=&terrains[i];
        if(t->broken) continue;
        int sx=SX(t->base.x-t->base.col_w/2), sy=SY(t->base.y-t->base.col_h/2);
        int sw2=SP(t->base.col_w), sh2=SP(t->base.col_h);
        if(CULL(sx,sy,sw2,sh2)) continue;

        if(t->type==TERRAIN_BREAK){
            draw_rect(&window,sx,sy,sw2,sh2,0x9B6B4BFF);
            draw_line(&window,sx+4,sy+4,sx+sw2-4,sy+sh2-4,0x5A3A2AFF);
            draw_line(&window,sx+sw2-4,sy+4,sx+4,sy+sh2-4,0x5A3A2AFF);
        } else {
            double h = cfg.world_h - t->base.y;
            unsigned int body = h>70 ? 0xDCDCECFF : h>40 ? 0x8A8A9AFF : 0x5A5A6AFF;
            unsigned int top  = h>70 ? 0xFFFFFFFF : h>40 ? 0xBBBBCCFF : 0x777788FF;
            draw_rect(&window,sx,sy,sw2,sh2,body);
            draw_rect(&window,sx,sy,sw2,3,top);
            draw_rect(&window,sx,sy+sh2-3,sw2,3,0x00000033);
        }
        if(t->warning_timer>0 && ((int)(tps_timer.time*8)%2==0))
            draw_rect(&window,sx,sy,sw2,sh2,0xFF000077);
    }
}

//###############################################
// PLAYER
//###############################################
void draw_player(){
    if(player.invincible>0 && (player.invincible/3)%2==0) return;

    int spx=SX(player.base.x), spy=SY(player.base.y);
    int pw=SP(cfg.player_w), ph=SP(cfg.player_h);
    int hw=SP(0.20), hh=SP(0.18), es=SP(0.05);

    unsigned int body = 0x4169E1FF;
    if(player.edge_grab)              body=0x00CED1FF;
    else if(player.jump_boost_timer>0)  body=0xFFAA00FF;
    else if(player.speed_boost_timer>0) body=0x00FFCCFF;

    draw_rect_centered(&window,spx,spy,pw,ph,body);
    draw_rect_centered(&window,spx,spy-ph/2-hh/2,hw,hh,0xFFD700FF);

    int ex = (player.last_move_dir>0) ? spx+es : spx-es;
    draw_rect_centered(&window,ex,spy-ph/2-hh/2-es/2,es,es,0x222222FF);

    if(player.dashing>0){
        for(int k=1;k<=3;k++){
            draw_rect_centered(&window,
                spx - player.dash_dir*k*SP(0.14), spy,
                pw-k*SP(0.05), ph-k*SP(0.05), 0xFFFFFF22);
        }
    }
    if(player.edge_grab){
        int hx=spx+player.grab_wall_dir*(pw/2+SP(0.03)), hs=SP(0.06);
        draw_rect_centered(&window,hx,spy-hs,hs,SP(0.05),0xFFD700FF);
        draw_rect_centered(&window,hx,spy+hs,hs,SP(0.05),0xFFD700FF);
    }
}

void draw_player_sensors(){
    unsigned int c_off=0xFFFFFF33;
    struct player_sensor_data* s=&player.sensors;
#define DRAW_SENSOR(px,py,pw2,ph2,active,col) \
    draw_rect_centered(&window,SX(px),SY(py),SP(pw2),SP(ph2),(active)?(col):c_off)
    DRAW_SENSOR(s->dx,s->dy,s->dw,s->dh,s->d_active,0x00FF00FF);
    DRAW_SENSOR(s->ux,s->uy,s->uw,s->uh,s->u_active,0xFF4444FF);
    DRAW_SENSOR(s->lx,s->ly,s->lw,s->lh,s->l_active,0x4488FFFF);
    DRAW_SENSOR(s->rx,s->ry,s->rw,s->rh,s->r_active,0xFFAA00FF);
#undef DRAW_SENSOR
}

//###############################################
// ENEMIES
//###############################################
static inline void draw_eyes_cfg(int x,int y,int w,int h,const enemy_cfg_t* c){
    if(!c->draw_eyes) return;
    int s=SP(c->eye_size);
    draw_rect_centered(&window,x-w/5,y-h/5,s,s,0xFFFF00FF);
    draw_rect_centered(&window,x+w/5,y-h/5,s,s,0xFFFF00FF);
}

static inline void draw_hpbar_cfg(struct enemy_data* e,const enemy_cfg_t* c,int x,int y,int h){
    if(e->hp<=0||c->max_hp<=0) return;
    int bw=SP(c->hpbar_w), bh=SP(c->hpbar_h);
    int bx=x-bw/2,        by=y-h/2-SP(c->hpbar_yoff);
    draw_rect(&window,bx,by,bw,bh,0x333333FF);
    draw_rect(&window,bx,by,bw*e->hp/c->max_hp,bh,0xFF3333F0);
}

static inline unsigned int enemy_color(struct enemy_data* e,const enemy_cfg_t* c){
    if(e->dashing) return c->col_dash;
    if(c->col_lowhp && e->hp<=c->max_hp/2) return c->col_lowhp;
    return c->col_base;
}

static void draw_weather_fx(struct enemy_data* e,int i){
    double t = tps_timer.time;
    double ox = t * 2.5;          // steady rightward drift
    double oy = t * 3.0;          // steady downward drift
    double bx = e->base.x, by = e->base.y;
    for(int k=0;k<120;k++){
        double nx=fmod(fabs(sin(k*99.123)*12345.67),36.0);
        double ny=fmod(fabs(cos(k*88.321)*76543.21),36.0);
        double mx=fmod(nx+ox,36.0); if(mx<0) mx+=36.0; mx-=18.0;
        double my=fmod(ny+oy,36.0); if(my<0) my+=36.0; my-=18.0;
        if(mx*mx+my*my<324.0){
            unsigned int col=(k%3==0)?0x228B22FF:(k%3==1)?0x32CD32FF:0x8B4513FF;
            if(k&1) draw_rect(&window,SX(bx+mx),SY(by+my),6,10,col);
            else    draw_rect(&window,SX(bx+mx),SY(by+my),10,6,col);
        }
    }
}

void draw_enemies(enemy_cfg_t* tbl){
    for(int i=0;i<enemy_count_actual;i++){
        struct enemy_data* e=&enemies[i];
        const enemy_cfg_t* c=&tbl[e->type];
        int x=SX(e->base.x), y=SY(e->base.y);
        if(y<-c->cull_margin||y>SH+c->cull_margin) continue;
        int w=SP(e->base.col_w), h=SP(e->base.col_h);

        if(!e->active){
            draw_rect_centered(&window,x,y,w,h,0x666666FF);
            draw_line(&window,x-w/5,y-h/5,x+w/5,y+h/5,0x222222FF);
            draw_line(&window,x+w/5,y-h/5,x-w/5,y+h/5,0x222222FF);
            continue;
        }
        draw_rect_centered(&window,x,y,w,h,enemy_color(e,c));
        draw_eyes_cfg(x,y,w,h,c);
        draw_hpbar_cfg(e,c,x,y,h);
        if(c->has_weather_fx) draw_weather_fx(e,i);
    }
}

//###############################################
// COINS
//###############################################
void draw_coins(){
    for(int i=0;i<coin_count_actual;i++){
        if(coins[i].collected) continue;
        int cx=SX(coins[i].x), cy=SY(coins[i].y);
        if(cy<-20||cy>SH+20) continue;
        draw_rect_centered(&window,cx,cy,SP(0.16),SP(0.16),0xFFD700FF);
        draw_rect_centered(&window,cx,cy,SP(0.09),SP(0.09),0xFFF5B0FF);
    }
}

//###############################################
// ITEMS
//###############################################
void draw_items(){
    static const unsigned int ITEM_COLS[4][3] = {
        {0,0,0},                          // unused (type 0)
        {0x00FFFFFF,0x00FFFFFF,0x00AAFFFF}, // type 1 - speed
        {0xFFFF00FF,0xFFFF00FF,0xFFAA00FF}, // type 2 - jump
        {0xCC1100FF,0xFF4400FF,0xFFFF00FF}, // type 3 - fireball
    };
    for(int i=0;i<item_count_actual;i++){
        if(!items[i].active) continue;
        int ix=SX(items[i].x), iy=SY(items[i].y);
        if(iy<-20||iy>SH+20) continue;
        int t=items[i].type;
        if(t>=1&&t<=3){
            const unsigned int* col=ITEM_COLS[t];
            if(t==3){
                draw_rect_centered(&window,ix,iy,SP(0.22),SP(0.22),col[0]);
                draw_rect_centered(&window,ix,iy,SP(0.16),SP(0.16),col[1]);
                draw_rect_centered(&window,ix,iy,SP(0.08),SP(0.08),col[2]);
            } else {
                draw_rect_centered(&window,ix,iy,SP(0.22),SP(0.08),col[0]);
                draw_rect_centered(&window,ix,iy,SP(0.08),SP(0.22),col[1]);
                draw_rect_centered(&window,ix,iy,SP(0.14),SP(0.14),col[2]);
            }
        }
        draw_rect_centered(&window,ix,iy,SP(0.06),SP(0.06),0xFFFFFFFF);
    }
}

//###############################################
// DECORS
//###############################################
void draw_decors(){
    for(int i=0;i<decor_count_actual;i++){
        struct decor_data* d=&decors[i];
        int dx=SX(d->x), dy=SY(d->y), dw=SP(d->w), dh=SP(d->h);
        if(CULL(dx-dw/2,dy-dh/2,dw,dh)) continue;

        if(d->is_teleporter){
            draw_rect_centered(&window,dx,dy,dw+SP(0.12),dh+SP(0.12),0x00FFFF22);
            draw_rect_centered(&window,dx,dy,dw+SP(0.06),dh+SP(0.06),0x00FFFF11);
            // frame
            draw_rect_centered(&window,dx,          dy,              dw,     SP(0.04),0x88EEFFFF);
            draw_rect_centered(&window,dx,          dy+dh/2-SP(0.02),dw,     SP(0.04),0x88EEFFFF);
            draw_rect_centered(&window,dx-dw/2+SP(0.02),dy,          SP(0.04),dh,    0x88EEFFFF);
            draw_rect_centered(&window,dx+dw/2-SP(0.02),dy,          SP(0.04),dh,    0x88EEFFFF);
            // bars
            draw_rect_centered(&window,dx-dw/5,dy,SP(0.03),dh-SP(0.04),0x66CCDDFF);
            draw_rect_centered(&window,dx,     dy,SP(0.03),dh-SP(0.04),0x66CCDDFF);
            draw_rect_centered(&window,dx+dw/5,dy,SP(0.03),dh-SP(0.04),0x66CCDDFF);
            // glow core
            draw_rect_centered(&window,dx,dy,dw/2,dh/2,0x00FFFF30);
            draw_rect_centered(&window,dx,dy,dw/4,dh/4,0x00FFFFFF);
            draw_text_centered(&window,&font,"ENTER",dx,dy-dh/2-SP(0.15),2,2,2,2,0x00FFDDFF);
        } else {
            draw_rect_centered(&window,dx,dy,dw,dh,d->color);
            if(d->label[0]) draw_text_centered(&window,&font,d->label,dx,dy-4,2,2,2,2,0x3B2200FF);
        }
    }
}

//###############################################
// PUSHABLE OBJECTS
//###############################################
void draw_pobjs(){
    for(int i=0;i<pobj_count_actual;i++){
        struct pobj_data* p=&pobjs[i];
        if(!p->active) continue;
        int px=SX(p->base.x), py=SY(p->base.y);
        int pw=SP(p->base.col_w), ph=SP(p->base.col_h);
        if(CULL(px-pw/2,py-ph/2,pw,ph)) continue;
        draw_rect_centered(&window,px,py,pw,ph,p->color);
        draw_rect_centered(&window,px,py-ph/2+1,pw,2,0xFFFFFF44);
        draw_rect_centered(&window,px,py+ph/2-1,pw,2,0x00000044);
    }
}

//###############################################
// PROJECTILES
//###############################################
void draw_projectiles(){
    for(int i=0;i<PROJ_COUNT;i++){
        if(!projectiles[i].active) continue;
        int px=SX(projectiles[i].x), py=SY(projectiles[i].y);
        if(projectiles[i].type==0){
            draw_rect_centered(&window,px,py,SP(0.2),SP(0.2),0xFF4500FF);
            draw_rect_centered(&window,px,py,SP(0.1),SP(0.1),0xFFD700FF);
        } else {
            draw_rect_centered(&window,px,py,SP(0.2),SP(0.2),0x8A2BE2FF);
            draw_rect_centered(&window,px,py,SP(0.1),SP(0.1),0x00FF00FF);
        }
    }
}

//###############################################
// HUD
//###############################################
void draw_hud(){
    char buf[128];
    int cx=SW/2;
    int pad=SW*7/1000, row_h=SH*3/100, bar_w=SW*11/100;

// top-left: coins / hp / ammo
#define HUD_ROW(yoff,fmt,val,col) \
    draw_rect(&window,pad,pad+(yoff),bar_w,row_h,0x00000088); \
    sprintf(buf,fmt,val); \
    draw_text(&window,&font,buf,pad+4,pad+(yoff)+4,2,2,2,2,col)
    HUD_ROW(0,          "COINS %d", player.score,         0xFFD700FF);
    HUD_ROW(row_h+4,    "HP: %d",   player.hp,            0xFF3333FF);
    HUD_ROW(row_h*2+8,  "FIRE: %d", player.fireball_ammo, 0xFF6600FF);
#undef HUD_ROW

    // top-center: position block + progress bar
    double h_m=cfg.world_h-player.base.y;
    if(h_m<0) h_m=0; if(h_m>cfg.world_h) h_m=cfg.world_h;
    double x_m=player.base.x;
    const char* ns=(h_m>cfg.world_h*0.70)?"N":(h_m<cfg.world_h*0.20)?"S":"-";
    const char* ew=(x_m<cfg.world_w*0.20)?"W":(x_m>cfg.world_w*0.70)?"E":"-";
    sprintf(buf,"%s%s  X%.1fm  Y%.1fm",ns,ew,x_m,h_m);
    int blk_w=SW*22/100, blk_h=SH*58/1000, blk_x=cx-blk_w/2;
    draw_rect(&window,blk_x,6,blk_w,blk_h,0x00000099);
    draw_text_centered(&window,&font,buf,cx,10,1,1,2,1,0xAADDFFFF);
    sprintf(buf,"%.0fm",h_m);
    draw_text_centered(&window,&font,buf,cx,24,2,2,2,2,0xFFFFFFFF);
    int pbw=SW*13/100, pbh=SH*9/1000, pbx=cx-pbw/2, pby=40;
    draw_rect(&window,pbx,pby,pbw,pbh,0x333333FF);
    draw_rect(&window,pbx,pby,(int)(pbw*h_m/cfg.world_h),pbh,0x00FF88FF);
    draw_rect(&window,pbx,pby,pbw,2,0xFFFFFF33);

    // boost badges
    int hud_y=blk_h+10;
    if(player.jump_boost_timer>0){
        draw_rect(&window,cx-SW*7/100,hud_y,SW*6/100,SH*23/1000,0xFFAA0099);
        draw_text_centered(&window,&font,"JUMP",cx-SW*4/100,hud_y+SH*14/1000,1,1,2,1,0xFFFFFFFF);
    }
    if(player.speed_boost_timer>0){
        draw_rect(&window,cx+SW*1/100,hud_y,SW*6/100,SH*23/1000,0x00AAFF99);
        draw_text_centered(&window,&font,"SPEED",cx+SW*4/100,hud_y+SH*14/1000,1,1,2,1,0xFFFFFFFF);
    }

    // top-right badges
    if(player.edge_grab){
        draw_rect(&window,SW-SW*6/100,SH*25/1000,SW*5/100,SH*25/1000,0x00CED188);
        draw_text(&window,&font,"GRAB",SW-SW*45/1000,SH*32/1000,1.2,1.2,2,1,0xFFFFFFFF);
    }
    if(player.god_mode){
        draw_rect(&window,SW-SW*92/1000,pad,SW*83/1000,row_h,0xFF880099);
        draw_text(&window,&font,"GOD",SW-SW*87/1000,pad+4,2,2,2,2,0xFFFFFFFF);
    }
}

//###############################################
// SHARED UI: BUTTON
//###############################################
static void draw_btn(int x,int y,int w,int h,const char* lbl,unsigned int col){
    int hov=(mouse.x>=x&&mouse.x<=x+w&&mouse.y>=y&&mouse.y<=y+h);
    draw_rect(&window,x,y,w,h,hov?(col|0x404040FF):col);
    draw_rect(&window,x,y,w,2,0xFFFFFF55);
    draw_rect(&window,x,y+h-2,w,2,0x00000055);
    draw_text_centered(&window,&font,lbl,x+w/2,y+h/2-6,2,2,2,2,0xFFFFFFFF);
}

//###############################################
// MENU
//###############################################
void draw_menu(){
    int cx=SW/2;
    draw_rect(&window,0,0,SW,SH,0x0A0F1AFF);

    // stars
    for(int i=0;i<80;i++){
        int sx2=(i*97+13)%SW, sy2=(i*137+41)%SH;
        int s=(i%3==0)?3:2;
        unsigned int ca=(i%3==0)?0xFFFFFFAAu:0xFFFFFF88u;
        unsigned int cb=(i%3==0)?0xFFFFFF66u:ca;
        draw_rect(&window,sx2-s,sy2,  s*2+1,1,ca);
        draw_rect(&window,sx2,  sy2-s,1,s*2+1,ca);
        if(i%3==0){
            draw_rect(&window,sx2-s+1,sy2-1,s*2-1,1,cb);
            draw_rect(&window,sx2-1,sy2-s+1,1,s*2-1,cb);
        }
    }

    draw_text_centered(&window,&font,"VERTICAL",       cx,SH*18/100,5,5,3,5,0xC8DCF0FF);
    draw_text_centered(&window,&font,"CLIMBER",        cx,SH*25/100,5,5,3,5,0x88BBDDFF);
    draw_text_centered(&window,&font,"REACH THE SUMMIT",cx,SH*33/100,1,1,2,1,0x7090B0FF);

    int bw=SW/6, bh=SH*6/100, bx=cx-bw/2;

    if(menu_sub_state==0){
        draw_btn(bx,SH*40/100,bw,bh,"NEW GAME",0x1A4A2AFF);
        draw_btn(bx,SH*48/100,bw,bh,"OPTIONS", 0x1A2A4AFF);
        draw_btn(bx,SH*56/100,bw,bh,"ABOUT",   0x2A1A4AFF);
        draw_btn(bx,SH*64/100,bw,bh,"EXIT",    0x4A1A1AFF);
        draw_text_centered(&window,&font,"ENTER FOR NEW GAME",cx,SH*72/100,1,1,2,1,0x445566FF);
    } else if(menu_sub_state==1){
        draw_text_centered(&window,&font,"CONTROLS",cx,SH*39/100,2,2,2,2,0xC8DCF0FF);
        int lx=cx-SW/6, ly=SH*45/100, ls=SH*23/1000;
        static const char* CTRL_LINES[]={
            "A D      MOVE","SPACE    JUMP","SHIFT    DASH",
            "A OR D   WALL GRAB","G        GOD MODE",
            "K        RELOAD WORLD","ESC      PAUSE",NULL,"ESC      BACK"
        };
        for(int r=0;r<9;r++)
            if(CTRL_LINES[r])
                draw_text(&window,&font,CTRL_LINES[r],lx,ly+ls*r,1,1,2,1,
                    r==8?0x8899AAFF:0xFFFFFFFF);
    } else {
        draw_text_centered(&window,&font,"ABOUT",cx,SH*39/100,2,2,2,2,0xC8DCF0FF);
        int lx=cx-SW/6, ly=SH*45/100, ls=SH*25/1000;
        static const char* ABOUT_LINES[]={
            "MADE WITH SDL2 AND C","CLIMB 100M TO WIN","STOMP ENEMIES",
            "BREAK CRACKED ROCKS","COLLECT GOLD COINS","GET STAR FOR BOOST",
            NULL,"ESC  BACK"
        };
        for(int r=0;r<8;r++)
            if(ABOUT_LINES[r])
                draw_text(&window,&font,ABOUT_LINES[r],lx,ly+ls*r,1,1,2,1,
                    r==7?0x8899AAFF:0xFFFFFFFF);
    }
}

//###############################################
// CHESTS
//###############################################
void draw_chests(){
    for(int i=0;i<chest_count_actual;i++){
        int cx=SX(chests[i].x), cy=SY(chests[i].y);
        int cw=SP(0.6), ch=SP(0.4);
        if(chests[i].state==0){
            draw_rect_centered(&window,cx,cy,cw,ch,0x8B4513FF);
            draw_rect_centered(&window,cx,cy,cw+2,4,0x000000FF);
            draw_rect_centered(&window,cx,cy,6,8,0xFFD700FF);
        } else {
            draw_rect_centered(&window,cx,cy+ch/4,cw,ch/2,0x8B4513FF);
            draw_rect_centered(&window,cx,cy-ch/4,cw,ch/4,0x6B3A2AFF);
        }
        if(chests[i].show_key){
            draw_rect_centered(&window,cx,cy-SP(0.6),SP(0.3),SP(0.3),0x00000088);
            draw_text_centered(&window,&font,"E",cx,cy-SP(0.6)-5,2,2,2,2,0xFFFFFFFF);
        }
    }
}

//###############################################
// WIN / PAUSE SCREENS
//###############################################
void draw_win_screen(){
    int cx=SW/2, bw=SW/6, bh=SH*6/100, bx=cx-bw/2;
    draw_rect(&window,0,0,SW,SH,0x00000099);
    draw_text_centered(&window,&font,"SUMMIT REACHED",cx,SH*28/100,4,4,3,4,0xFFFF88FF);
    char buf[32]; sprintf(buf,"COINS %d",player.score);
    draw_text_centered(&window,&font,buf,cx,SH*42/100,2,2,2,2,0xFFD700FF);
    draw_btn(bx,SH*50/100,bw,bh,"PLAY AGAIN",0x1A4A2AFF);
    draw_btn(bx,SH*58/100,bw,bh,"MAIN MENU", 0x2A2A4AFF);
    draw_text_centered(&window,&font,"SPACE TO REPLAY",cx,SH*68/100,1,1,2,1,0x667788FF);
}

void draw_pause_screen(){
    int cx=SW/2, bw=SW/6, bh=SH*6/100, bx=cx-bw/2;
    draw_rect(&window,0,0,SW,SH,0x00000099);
    draw_text_centered(&window,&font,"PAUSED",cx,SH*42/100,4,4,2,4,0xFFFFFFFF);
    draw_text_centered(&window,&font,"ESC TO RESUME",cx,SH*50/100,1,1,2,1,0x8899AAFF);
    draw_btn(bx,SH*56/100,bw,bh,"MAIN MENU",0x2A2A4AFF);
}

#endif