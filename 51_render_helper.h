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

#define WRECT(wx,wy,ww,wh,col) \
    draw_rect_centered(&window, SX(wx), SY(wy), SP(ww), SP(wh), col)

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
// PLAYER RENDERING  (shared for P1 and P2)
// is_p2_colors: 1 = orange body/green cap, 0 = blue body/gold cap
//###############################################
static void draw_player_gfx(struct player_data* p, int is_p2_colors, const char* label){
    if(p->invincible>0 && (p->invincible/3)%2==0) return;

    int spx=SX(p->base.x), spy=SY(p->base.y);
    int pw=SP(cfg.player_w), ph=SP(cfg.player_h);
    int hw=SP(0.20), hh=SP(0.18), es=SP(0.05);

    unsigned int body = is_p2_colors ? 0xE06020FF : 0x4169E1FF;
    unsigned int cap  = is_p2_colors ? 0x44CC44FF : 0xFFD700FF;
    if(p->edge_grab)              body = is_p2_colors ? 0x20D0D0FF : 0x00CED1FF;
    else if(p->jump_boost_timer>0)  body = is_p2_colors ? 0xFFCC00FF : 0xFFAA00FF;
    else if(p->speed_boost_timer>0) body = is_p2_colors ? 0x20FFAAFF : 0x00FFCCFF;

    draw_rect_centered(&window,spx,spy,pw,ph,body);
    draw_rect_centered(&window,spx,spy-ph/2-hh/2,hw,hh,cap);

    int ex=(p->last_move_dir>0) ? spx+es : spx-es;
    draw_rect_centered(&window,ex,spy-ph/2-hh/2-es/2,es,es,0x222222FF);

    if(p->dashing>0){
        for(int k=1;k<=3;k++)
            draw_rect_centered(&window,
                spx - p->dash_dir*k*SP(0.14), spy,
                pw-k*SP(0.05), ph-k*SP(0.05), 0xFFFFFF22);
    }
    if(p->edge_grab){
        int hx=spx+p->grab_wall_dir*(pw/2+SP(0.03)), hs=SP(0.06);
        draw_rect_centered(&window,hx,spy-hs,hs,SP(0.05),cap);
        draw_rect_centered(&window,hx,spy+hs,hs,SP(0.05),cap);
    }
    if(label)
        draw_text_centered(&window,&font,label,spx,spy-ph/2-hh-SP(0.15),1,1,2,1,0xFFFFFFCC);
}

void draw_player(){
    // On client: local player IS p2 (orange). On host/solo: local player IS p1 (blue).
    draw_player_gfx(&player, lan.role==LAN_CLIENT, NULL);
}

void draw_p2(){
    if(lan.role==LAN_OFF) return;
    struct player_data* p=&lan.p2;
    if(p->hp<=0) return;
    // On client: lan.p2 is the host's character (p1, blue). On host: lan.p2 is client (orange).
    draw_player_gfx(p, lan.role==LAN_HOST, lan.role==LAN_CLIENT ? "P1" : "P2");
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

static void draw_weather_fx(struct enemy_data* e, int i){
    double t   = tps_timer.time;
    double dir = (e->dash_vx >= 0) ? 1.0 : -1.0;
    double spd = fabs(e->dash_vx) * ENEMY_CFG[e->type].weather_push_scale; // stronger = faster scroll
    double ox  = t * spd * dir;
    double oy  = t * 1.5;
    double bx  = e->base.x, by = e->base.y;

    for(int k=0;k<120;k++){
        double nx=fmod(fabs(sin(k*99.123)*12345.67),36.0);
        double ny=fmod(fabs(cos(k*88.321)*76543.21),36.0);
        double mx=fmod(nx+ox,36.0); if(mx<0) mx+=36.0; mx-=18.0;
        double my=fmod(ny+oy,36.0); if(my<0) my+=36.0; my-=18.0;
        if(mx*mx+my*my<324.0){
            unsigned int col=(k%3==0)?0x228B22FF:(k%3==1)?0x32CD32FF:0x8B4513FF;
            // wide+short = horizontal wind, tall+narrow = calm/vertical
            if(k&1) draw_rect(&window,SX(bx+mx),SY(by+my), (int)(10*fabs(dir)), 5, col);
            else    draw_rect(&window,SX(bx+mx),SY(by+my), (int)(12*fabs(dir)), 4, col);
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
        {0,0,0},
        {0x00FFFFFF,0x00FFFFFF,0x00AAFFFF},
        {0xFFFF00FF,0xFFFF00FF,0xFFAA00FF},
        {0xCC1100FF,0xFF4400FF,0xFFFF00FF},
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
            draw_rect_centered(&window,dx,          dy,              dw,     SP(0.04),0x88EEFFFF);
            draw_rect_centered(&window,dx,          dy+dh/2-SP(0.02),dw,     SP(0.04),0x88EEFFFF);
            draw_rect_centered(&window,dx-dw/2+SP(0.02),dy,          SP(0.04),dh,    0x88EEFFFF);
            draw_rect_centered(&window,dx+dw/2-SP(0.02),dy,          SP(0.04),dh,    0x88EEFFFF);
            draw_rect_centered(&window,dx-dw/5,dy,SP(0.03),dh-SP(0.04),0x66CCDDFF);
            draw_rect_centered(&window,dx,     dy,SP(0.03),dh-SP(0.04),0x66CCDDFF);
            draw_rect_centered(&window,dx+dw/5,dy,SP(0.03),dh-SP(0.04),0x66CCDDFF);
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
// HUD  — P1 top-left, P2 top-right when in LAN
//###############################################
void draw_hud(){
    char buf[128];
    int cx=SW/2;
    int pad=SW*7/1000, row_h=SH*3/100, bar_w=SW*11/100;

    // P1 — top-left
#define HUD_ROW(yoff,fmt,val,col) \
    draw_rect(&window,pad,pad+(yoff),bar_w,row_h,0x00000088); \
    sprintf(buf,fmt,val); \
    draw_text(&window,&font,buf,pad+4,pad+(yoff)+4,2,2,2,2,col)
    HUD_ROW(0,          "COINS %d", player.score,         0xFFD700FF);
    HUD_ROW(row_h+4,    "HP: %d",   player.hp,            0xFF3333FF);
    HUD_ROW(row_h*2+8,  "FIRE: %d", player.fireball_ammo, 0xFF6600FF);
#undef HUD_ROW

    // P2 — top-right (only in LAN)
    if(lan.role != LAN_OFF){
        int p2_x = SW - pad - bar_w;
#define HUD_ROW_P2(yoff,fmt,val,col) \
    draw_rect(&window,p2_x,pad+(yoff),bar_w,row_h,0x00000088); \
    sprintf(buf,fmt,val); \
    draw_text(&window,&font,buf,p2_x+4,pad+(yoff)+4,2,2,2,2,col)
        HUD_ROW_P2(0,         "COINS %d", lan.p2.score,         0xFFD700FF);
        HUD_ROW_P2(row_h+4,   "HP: %d",   lan.p2.hp,            0xE06020FF);
        HUD_ROW_P2(row_h*2+8, "FIRE: %d", lan.p2.fireball_ammo, 0xFF6600FF);
#undef HUD_ROW_P2
        draw_rect(&window,p2_x,pad,bar_w,row_h,0x00000088);
        draw_text(&window,&font, lan.role==LAN_CLIENT ? "P1" : "P2",
            p2_x+4,pad+4,2,2,2,2,0xFFFFFFCC);
    }

    // top-center: position + progress bar
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

    // top-right badges (only solo — in LAN the P2 panel is there instead)
    if(lan.role == LAN_OFF){
        if(player.edge_grab){
            draw_rect(&window,SW-SW*6/100,SH*25/1000,SW*5/100,SH*25/1000,0x00CED188);
            draw_text(&window,&font,"GRAB",SW-SW*45/1000,SH*32/1000,1.2,1.2,2,1,0xFFFFFFFF);
        }
        if(player.god_mode){
            draw_rect(&window,SW-SW*92/1000,pad,SW*83/1000,row_h,0xFF880099);
            draw_text(&window,&font,"GOD",SW-SW*87/1000,pad+4,2,2,2,2,0xFFFFFFFF);
        }
    }
}

//###############################################
// CHESTS
//###############################################
void draw_chests(){
double px=player.base.x, py=player.base.y;

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

		double dx = px - chests[i].x;
		double dy = py - chests[i].y;
		int player_near = (dx*dx + dy*dy < 1.5*1.5);

        if(chests[i].state != 1 && player_near){
            draw_rect_centered(&window,cx,cy-SP(0.6),SP(0.3),SP(0.3),0x00000088);
            draw_text_centered(&window,&font,"E",cx,cy-SP(0.6)-5,2,2,2,2,0xFFFFFFFF);
        }
    }
}


void draw_stars(){
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
return;
}

static inline void draw_full_scr_overlay(unsigned int col){
    unsigned int a = col & 0xFF;
    if(a == 0) return;

    draw_rect(&window, 0, 0, SW, SH, col);
}

void draw_menu(){
    // titles
    draw_btn(gui.menu_title_main);
    draw_btn(gui.menu_title_hint);

    if(menu_sub_state==0){
        draw_btn(gui.menu_new_game);
        draw_btn(gui.menu_options);
        draw_btn(gui.menu_about);
        draw_btn(gui.menu_host);
        draw_btn(gui.menu_join);
        draw_btn(gui.menu_exit);

        draw_btn(gui.menu_enter_hint);
    }
    else if(menu_sub_state==1){
        draw_btn(gui.menu_controls_title);
        for(int i=0;i<8;i++) draw_btn(gui.menu_ctrl[i]);
    }
    else{
        draw_btn(gui.menu_about_title);
        for(int i=0;i<7;i++) draw_btn(gui.menu_about_lines[i]);
    }
}



//###############################################
// WIN / PAUSE SCREENS
//###############################################
void draw_win_screen(){
    char buf[32]; sprintf(buf,"COINS %d",player.score);
    int i=0; for(; buf[i] && i<255; i++) gui.win_score.text[i]=buf[i];
    gui.win_score.text[i]=0;

    draw_btn(gui.win_title);
    draw_btn(gui.win_score);
	if(lan.connected && lan.role == LAN_HOST || !lan.connected){ draw_btn(gui.win_play_again); }
    draw_btn(gui.win_main_menu);
    draw_btn(gui.win_hint);
}

void draw_pause_screen(){
    draw_btn(gui.pause_title);
    draw_btn(gui.pause_hint);
    draw_btn(gui.pause_main_menu);
}

//###############################################
// HOST / JOIN SCREENS
//###############################################
static inline void lan_draw_lobby(lan_ctx_t* lan){
    static int c=0; c++;
    int blink = (c/30)%2;

    // reset dynamic colors (avoid sticking)
    gui.hint_status.text_col = 0xFFAA44FF;
    gui.hint_enter.text_col  = 0x44FF88FF;

    // title
    gui_set_text(&gui.title, lan->role==LAN_HOST?"HOST":"CLIENT");
    draw_btn(gui.title);

    // port
    draw_btn(gui.port_label);
    gui_field(&gui.port_box, lan->ui_port, lan->ui_focus==0, blink);

    // ip
    if(lan->role == LAN_CLIENT){
        draw_btn(gui.ip_label);
        gui_field(&gui.ip_box, lan->ui_ip, lan->ui_focus==1, blink);
        draw_btn(gui.hint_tab);
    }

    // status / enter
    if(!lan->sock_open){
        gui_set_text(&gui.hint_enter,
            lan->role==LAN_HOST?"ENTER  START HOSTING":"ENTER  CONNECT TO HOST");
        draw_btn(gui.hint_enter);
    }
    else if(!lan->connected){
        gui_set_text(&gui.hint_status, lan->status_msg);
        draw_btn(gui.hint_status);
    }
    else{
        gui.hint_status.text_col = 0xAADDFFFF;
        gui_set_text(&gui.hint_status, lan->status_msg);
        draw_btn(gui.hint_status);

        if(lan->role == LAN_HOST){
            gui_set_text(&gui.hint_enter, "ENTER  BEGIN GAME");
            draw_btn(gui.hint_enter);
        }
    }

    draw_btn(gui.hint_esc);

    /* CLIENT: poll discovery and draw visible host list */
    if(lan->role == LAN_CLIENT && !lan->sock_open){
        draw_btn(gui.disc_title);

        if(lan->discovered_count == 0){
            gui_rect_t empty = gui.disc_entries[0];
            gui_set_text(&empty, "Scanning...");
            empty.box_col  = 0x00000000;
            empty.text_col = 0x445566FF;
            draw_btn(empty);
        } else {
            for(int i = 0; i < lan->discovered_count && i < 8; i++){
                char label[48];
                snprintf(label, 48, "%s  :%d",
                    lan->discovered[i].ip,
                    (int)lan->discovered[i].port);
                gui_rect_t* r = &gui.disc_entries[i];
                gui_set_text(r, label);
                r->box_col = (lan->selected_host == i) ? 0x1A4A2AFF : 0x0D1A2AFF;
                draw_btn(*r);
            }
        }
    }
}
#endif