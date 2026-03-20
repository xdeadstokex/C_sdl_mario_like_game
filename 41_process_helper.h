#ifndef PROCESS_HELPER_H
#define PROCESS_HELPER_H
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include "10_data.h"
#include "external_lib/common_logic.h"
#include "external_lib/physic.h"

static inline double dclamp(double v,double lo,double hi){ return v<lo?lo:v>hi?hi:v; }
static inline double dsign(double v){ return v>0?1.0:v<0?-1.0:0.0; }

// forward declaration — defined in 20_init.h
void reset_player();

//###############################################
// PLAYER SENSORS
// 4 thin rects flush on each player face.
// Active flags persist from terrain pass into
// next movement frame — cleared inside movement
// after jump/grab checks consume them.
//###############################################
static inline void update_player_sensors(){
    double cx  = player.base.x + player.base.col_ox;
    double cy  = player.base.y + player.base.col_oy;
    double hw  = player.base.col_w * 0.5;
    double hh  = player.base.col_h * 0.5;
    double lrw = player.base.col_w * SENSOR_LR_W_PCT;
    double lrh = player.base.col_h * SENSOR_LR_H_PCT;
    double udw = player.base.col_w * SENSOR_UD_W_PCT;
    double udh = player.base.col_h * SENSOR_UD_H_PCT;
    player.sensors.lx=cx-hw; player.sensors.ly=cy; player.sensors.lw=lrw; player.sensors.lh=lrh;
    player.sensors.rx=cx+hw; player.sensors.ry=cy; player.sensors.rw=lrw; player.sensors.rh=lrh;
    player.sensors.ux=cx; player.sensors.uy=cy-hh; player.sensors.uw=udw; player.sensors.uh=udh;
    player.sensors.dx=cx; player.sensors.dy=cy+hh; player.sensors.dw=udw; player.sensors.dh=udh;
}

static inline void clear_sensor_flags(){
    player.sensors.l_active=0; player.sensors.r_active=0;
    player.sensors.u_active=0; player.sensors.d_active=0;
}

// Test all 4 sensors against a rect, write results and OR into active flags
static inline void sense_one(double tx, double ty, double tw, double th,
                              int* hd, int* hu, int* hl, int* hr){
    *hd=check_two_box_2d_hit_centralized(player.sensors.dx,player.sensors.dy,player.sensors.dw,player.sensors.dh,tx,ty,tw,th);
    *hu=check_two_box_2d_hit_centralized(player.sensors.ux,player.sensors.uy,player.sensors.uw,player.sensors.uh,tx,ty,tw,th);
    *hl=check_two_box_2d_hit_centralized(player.sensors.lx,player.sensors.ly,player.sensors.lw,player.sensors.lh,tx,ty,tw,th);
    *hr=check_two_box_2d_hit_centralized(player.sensors.rx,player.sensors.ry,player.sensors.rw,player.sensors.rh,tx,ty,tw,th);
    player.sensors.d_active|=*hd; player.sensors.u_active|=*hu;
    player.sensors.l_active|=*hl; player.sensors.r_active|=*hr;
}

//###############################################
// PLAYER STATE HELPERS
//###############################################
static inline double jump_vy(){
    return (player.jump_boost_timer>0) ? cfg.jump_boost_vy : cfg.jump_vy;
}

static inline void player_land(){
    player.on_ground=1; player.jump_count=0;
    player.dash_ready=1; player.edge_grab=0; player.grab_wall_dir=0;
}

static inline void player_do_jump(){
    player.base.vy=jump_vy();
    player.jump_count=1; player.on_ground=0;
}

static inline void player_do_wall_jump(){
    player.edge_grab=0; player.grab_wall_dir=0;
    player.base.ay=cfg.gravity;
    player.base.vy=jump_vy();
    player.base.vx=-player.on_wall*3.0;
    player.jump_count=1; player.on_wall=0;
}

static inline void player_apply_velocity_caps(){
    player.base.vx=dclamp(player.base.vx,-cfg.max_vx,cfg.max_vx);
    player.base.vy=dclamp(player.base.vy,-cfg.max_vy,cfg.max_vy);
    player.base.x =dclamp(player.base.x, cfg.player_w*0.5, cfg.world_w-cfg.player_w*0.5);
}

//###############################################
// MOVEMENT SUB-SYSTEMS
//###############################################
static inline void apply_edge_grab(double dt, int ml, int mr){
    int release=(player.grab_wall_dir==-1&&mr)||(player.grab_wall_dir==1&&ml);
    if(release||player.on_ground){
        player.edge_grab=0; player.grab_wall_dir=0; player.base.ay=cfg.gravity;
    } else {
        player.base.vx=0;
        player.base.vy*=pow(cfg.edge_grab_slide, dt*20.0);
        player.base.ay=0;
    }
}

static inline void apply_horizontal_move(double dt, int ml, int mr, double cur_speed){
    double tvx=0;
    if(ml){ tvx=-cur_speed; player.last_move_dir=-1; }
    if(mr){ tvx= cur_speed; player.last_move_dir= 1; }
    if(tvx!=0){
        double diff=tvx-player.base.vx;
        double step=cfg.move_accel*dt;
        player.base.vx+=(fabs(diff)<step)?diff:dsign(diff)*step;
    } else {
        player.base.vx*=cfg.move_friction;
        if(fabs(player.base.vx)<0.01) player.base.vx=0;
    }
}

static inline void apply_jump(){
    if(!kb.key_space.click) return;
    if(player.edge_grab){
        player_do_wall_jump();
    } else if(player.on_ground||player.sensors.d_active){
        player_do_jump();
    } else if(player.jump_count<1){
        player.base.vy=jump_vy();
        player.jump_count++;
    }
}

static inline void apply_dash(int ml, int mr){
    if(!((kb.key_shift_l.click||kb.key_shift_r.click)
         &&player.dash_ready&&!player.dashing&&!player.edge_grab)) return;
    player.dashing=cfg.dash_frames;
    player.dash_dir=player.last_move_dir;
    player.dash_ready=0;
}

//###############################################
// PLAYER MOVEMENT
//###############################################
void process_player_movement(double dt){
    int ml=kb.key_a.hold, mr=kb.key_d.hold;

    if(player.speed_boost_timer>0) player.speed_boost_timer--;
    double cur_speed=(player.speed_boost_timer>0)?cfg.move_speed*1.5:cfg.move_speed;

    // GOD MODE — free fly, bypass all physics
    if(player.god_mode){
        player.base.vx=0; player.base.vy=0; player.base.ay=0;
        double fly=6.0;
        if(ml)                player.base.x-=fly*dt;
        if(mr)                player.base.x+=fly*dt;
        if(kb.key_w.hold)     player.base.y-=fly*dt;
        if(kb.key_s.hold)     player.base.y+=fly*dt;
        if(kb.key_space.hold) player.base.y-=fly*dt;
        player.base.x=dclamp(player.base.x,0,cfg.world_w);
        player.base.y=dclamp(player.base.y,0,cfg.world_h);
        return;
    }

    // edge grab sustain/release (must run before gravity is set)
    if(player.edge_grab) apply_edge_grab(dt,ml,mr);
    else                 player.base.ay=cfg.gravity;

    // dash movement or normal horizontal move
    if(player.dashing>0){
        player.dashing--;
        player.base.vx=player.dash_dir*cfg.dash_speed;
        player.base.vy=0; player.base.ay=0;
    } else if(!player.edge_grab){
        apply_horizontal_move(dt,ml,mr,cur_speed);
    }

    // jump — reads d_active and on_ground from previous terrain pass
    apply_jump();

    // FIREBALL SHOOT — F key
    if(kb.key_f.click && player.fireball_ammo>0){
        player.fireball_ammo--;
        for(int _i=0;_i<PROJ_COUNT;_i++){
            if(!projectiles[_i].active){
                projectiles[_i].active=1;
                projectiles[_i].x=player.base.x;
                projectiles[_i].y=player.base.y;
                projectiles[_i].vx=player.last_move_dir*12.0;
                projectiles[_i].vy=0;
                projectiles[_i].dir=player.last_move_dir;
                break;
            }
        }
    }

    // clear flags after jump consumed them; terrain pass repopulates this frame
    clear_sensor_flags();

    apply_dash(ml,mr);

    update_base(&player.base,dt);
    update_player_sensors();
    player_apply_velocity_caps();

    if(player.invincible>0)       player.invincible--;
    if(player.jump_boost_timer>0) player.jump_boost_timer--;
}

//###############################################
// TERRAIN COLLISION HELPERS
//###############################################
static inline void handle_terrain_break(int i, double ty, double th){
    double oy=(player.base.col_h+th)*0.5-fabs(player.base.y-ty);
    player.base.y-=oy;
    if(--terrains[i].hp<=0){
        terrains[i].broken=1;
        terrains[i].broken_timer=300*cfg.tps/20;
        player.base.vy=-2.5;
    } else { player.base.vy=0; }
    player_land();
    update_player_sensors();
}

static inline void handle_wall_contact(int hl, int hr, double ty, double th){
    int ws=hr?1:-1;
    player.on_wall=ws;
    int pressing=(ws==1&&kb.key_d.hold)||(ws==-1&&kb.key_a.hold);
    double terrain_top=ty-th*0.5;
    int beside=(player.base.y+player.base.col_oy)>=terrain_top;
    if(!player.on_ground&&pressing&&!player.edge_grab&&beside){
        player.edge_grab=1;
        player.grab_wall_dir=ws;
        player.base.vy=dclamp(player.base.vy,-2.0,2.0);
    }
}

//###############################################
// TERRAIN COLLISION  (player)
//###############################################
void process_terrain_collision(){
    if(player.god_mode) return;
    player.on_ground=0; player.on_wall=0;
    update_player_sensors();

    for(int i=0;i<terrain_count_actual;i++){
        if(terrains[i].broken) continue;
        double tx=terrains[i].base.x, ty=terrains[i].base.y;
        double tw=terrains[i].base.col_w, th=terrains[i].base.col_h;

        int hd,hu,hl,hr;
        sense_one(tx,ty,tw,th,&hd,&hu,&hl,&hr);
        if(!hd&&!hu&&!hl&&!hr) continue;

        if(hd&&terrains[i].type==TERRAIN_BREAK&&terrains[i].hp>0){
            handle_terrain_break(i,ty,th);
            continue;
        }

        double vx_before=player.base.vx;
        int hit=resolve_terrain_collision(&player.base,tx,ty,tw,th);
        if(!hit) continue;
        if(hit&0x1) player.base.vx=vx_before; // floor: don't let terrain friction kill hspeed
        update_player_sensors();

        if(hd) player_land();
        if(hl||hr) handle_wall_contact(hl,hr,ty,th);
    }

    if(player.on_ground)        player.edge_grab=0;
    if(player.sensors.u_active){ player.edge_grab=0; player.grab_wall_dir=0; }
}

//###############################################
// TERRAIN TIMERS
//###############################################
void process_terrain_timers(){
    for(int i=0;i<terrain_count_actual;i++){
        if(!terrains[i].broken) continue;
        if(--terrains[i].broken_timer<=0){ terrains[i].broken=0; terrains[i].hp=1; }
    }
}

//###############################################
// POBJ PHYSICS HELPERS
//###############################################
static inline void pobj_vs_terrain(struct pobj_data* p){
    p->on_ground=0;
    for(int j=0;j<terrain_count_actual;j++){
        if(terrains[j].broken) continue;
        int hit=resolve_terrain_collision(&p->base,
            terrains[j].base.x,terrains[j].base.y,
            terrains[j].base.col_w,terrains[j].base.col_h);
        if(hit&0x1) p->on_ground=1;
    }
}

static inline void player_vs_pobj(struct pobj_data* p){
    update_player_sensors();
    double bx=p->base.x+p->base.col_ox, by=p->base.y+p->base.col_oy;
    double bw=p->base.col_w,             bh=p->base.col_h;
    int hd,hu,hl,hr;
    sense_one(bx,by,bw,bh,&hd,&hu,&hl,&hr);

    if(hd&&player.base.vy>=0){
        double ov=(player.sensors.dh+bh)*0.5-fabs(player.sensors.dy-by);
        if(ov>0){ player.base.y-=ov; player.base.vy=0; player_land(); update_player_sensors(); }
    } else if(hu&&player.base.vy<0){
        double ov=(player.sensors.uh+bh)*0.5-fabs(player.sensors.uy-by);
        if(ov>0){ player.base.y+=ov; player.base.vy=0; update_player_sensors(); }
    } else if(hl){
        double ov=(player.sensors.lw+bw)*0.5-fabs(player.sensors.lx-bx);
        if(ov>0){ player.base.x+=ov; p->base.x-=ov; p->base.vx=player.base.vx; update_player_sensors(); }
    } else if(hr){
        double ov=(player.sensors.rw+bw)*0.5-fabs(player.sensors.rx-bx);
        if(ov>0){ player.base.x-=ov; p->base.x+=ov; p->base.vx=player.base.vx; update_player_sensors(); }
    }
}

//###############################################
// POBJS
//###############################################
void process_pobjs(double dt){
    for(int i=0;i<pobj_count_actual;i++){
        struct pobj_data* p=&pobjs[i];
        if(!p->active) continue;
        p->base.ay=cfg.gravity;
        update_base(&p->base,dt);
        p->base.vx=dclamp(p->base.vx,-cfg.max_vx,cfg.max_vx);
        p->base.vy=dclamp(p->base.vy,-cfg.max_vy,cfg.max_vy);
        pobj_vs_terrain(p);
        player_vs_pobj(p);
    }
}

//###############################################
// ENEMY HELPERS
//###############################################
static inline void enemy_update_patrol(struct enemy_data* e, double dt){
    double spd=1.0;
    e->base.x+=e->patrol_dir*spd*dt;
    if(e->base.x>=e->patrol_x_max){ e->base.x=e->patrol_x_max; e->patrol_dir=-1; }
    if(e->base.x<=e->patrol_x_min){ e->base.x=e->patrol_x_min; e->patrol_dir= 1; }
}

static inline void enemy_update_dash(struct enemy_data* e, double dt, double px){
    if(--e->dash_timer<=0&&!e->dashing){
        int tps=(int)(1.0/dt+0.5);
        int freq=(e->type==ENEMY_BOSS)?50:100;
        e->dash_timer=(freq-20+(rand()%40))*tps/20;
        e->dashing=tps*12/20;
        e->dash_vx=(px>e->base.x)?fabs(e->dash_vx):-fabs(e->dash_vx);
    }
    if(e->dashing>0){
        e->base.x+=e->dash_vx*dt;
        e->dashing--;
        e->base.x=dclamp(e->base.x,e->patrol_x_min,e->patrol_x_max);
    }
}

static inline void enemy_vs_player(struct enemy_data* e, double px, double py){
    double ew=e->base.col_w, eh=e->base.col_h;
    int stomp=check_two_box_2d_hit_centralized(
                  player.sensors.dx,player.sensors.dy,player.sensors.dw,player.sensors.dh,
                  e->base.x,e->base.y,ew,eh) && player.base.vy>0;
    int body=!stomp&&check_two_box_2d_hit_centralized(
                  px,py,cfg.player_w,cfg.player_h,e->base.x,e->base.y,ew,eh);
    if(!stomp&&!body) return;

    if(stomp){
        if(--e->hp<=0){ e->active=0; e->stun_timer=(e->type==ENEMY_BOSS)?99999:60*cfg.tps/20; }
        else           { e->active=0; e->stun_timer=(e->type==ENEMY_BOSS)?80*cfg.tps/20:60*cfg.tps/20; }
        player.base.vy=(player.jump_boost_timer>0)?cfg.jump_boost_vy*0.7:cfg.jump_vy*0.7;
    } else if(player.invincible==0){
        player.invincible=cfg.invincible_frames;
        player.hp--;
        if(player.hp<=0){
            reset_player();
        } else {
            double push=dsign(px-e->base.x); if(push==0) push=1;
            player.base.vx=push*((e->type==ENEMY_BOSS)?5.0:4.0);
            player.base.vy=(e->type==ENEMY_BOSS)?-4.5:-3.5;
        }
    }
}

//###############################################
// PROJECTILES (fireballs)
//###############################################
void process_projectiles(double dt){
    for(int i=0;i<PROJ_COUNT;i++){
        struct projectile_data* pr=&projectiles[i];
        if(!pr->active) continue;

        pr->x+=pr->vx*dt;
        pr->y+=pr->vy*dt;

        // deactivate if out of world bounds
        if(pr->x<0||pr->x>cfg.world_w||pr->y<0||pr->y>cfg.world_h){
            pr->active=0; continue;
        }

        // hit terrain
        int hit_terrain=0;
        for(int j=0;j<terrain_count_actual;j++){
            if(terrains[j].broken) continue;
            if(check_two_box_2d_hit_centralized(
                pr->x,pr->y,0.2,0.2,
                terrains[j].base.x,terrains[j].base.y,
                terrains[j].base.col_w,terrains[j].base.col_h)){
                hit_terrain=1; break;
            }
        }
        if(hit_terrain){ pr->active=0; continue; }

        // hit enemy
        for(int j=0;j<enemy_count_actual;j++){
            struct enemy_data* e=&enemies[j];
            if(!e->active) continue;
            if(check_two_box_2d_hit_centralized(
                pr->x,pr->y,0.2,0.2,
                e->base.x,e->base.y,e->base.col_w,e->base.col_h)){
                pr->active=0;
                if(--e->hp<=0){
                    e->active=0;
                    e->stun_timer=(e->type==ENEMY_BOSS)?99999:60*cfg.tps/20;
                } else {
                    e->active=0;
                    e->stun_timer=(e->type==ENEMY_BOSS)?80*cfg.tps/20:60*cfg.tps/20;
                }
                break;
            }
        }
    }
}

//###############################################
// ENEMIES
//###############################################
void process_enemies(double dt){
    double px=player.base.x, py=player.base.y;
    for(int i=0;i<enemy_count_actual;i++){
        struct enemy_data* e=&enemies[i];
        if(!e->active){
            if(e->type==ENEMY_BOSS&&e->hp<=0) continue;
            if(--e->stun_timer<=0){
                e->active=1;
                e->base.x=(e->patrol_x_min+e->patrol_x_max)*0.5;
            }
            continue;
        }
        if(!e->dashing) enemy_update_patrol(e,dt);
        enemy_update_dash(e,dt,px);
        e->base.y=e->patrol_y;
        enemy_vs_player(e,px,py);
    }
}

//###############################################
// COINS
//###############################################
void process_coins(){
    for(int i=0;i<coin_count_actual;i++){
        if(coins[i].collected) continue;
        if(check_two_box_2d_hit_centralized(
            player.base.x,player.base.y,cfg.player_w,cfg.player_h,
            coins[i].x,coins[i].y,0.24,0.24)){
            coins[i].collected=1; player.score++;
        }
    }
}

//###############################################
// ITEMS
//###############################################
void process_items(){
    for(int i=0;i<item_count_actual;i++){
        if(!items[i].active){
            if(items[i].respawn_timer>0&&--items[i].respawn_timer==0) items[i].active=1;
            continue;
        }
        if(check_two_box_2d_hit_centralized(
            player.base.x,player.base.y,cfg.player_w,cfg.player_h,
            items[i].x,items[i].y,0.24,0.24)){
            items[i].active=0;
            if(items[i].respawn_timer!=-1) items[i].respawn_timer=400*cfg.tps/20;
            if     (items[i].type==1) player.speed_boost_timer=200*cfg.tps/20;
            else if(items[i].type==2) player.jump_boost_timer =200*cfg.tps/20;
            else if(items[i].type==3) player.fireball_ammo    +=3;
        }
    }
}

//###############################################
// CHEST
//###############################################
void process_chests(){
    double px=player.base.x, py=player.base.y;
    for(int i=0;i<chest_count_actual;i++){
        if(chests[i].state==1){ chests[i].show_key=0; continue; }
        double dx=px-chests[i].x, dy=py-chests[i].y;
        chests[i].show_key=(dx*dx+dy*dy<1.5*1.5);
        if(!chests[i].show_key||!kb.key_e.click) continue;
        chests[i].state=1; chests[i].show_key=0;
        int j=0;
        for(;j<item_count_actual;j++) if(!items[j].active) break;
        if(j==item_count_actual&&item_count_actual<ITEM_COUNT) item_count_actual++;
        if(j<ITEM_COUNT){
            items[j].active=1;
            items[j].x=chests[i].x-0.3; items[j].y=chests[i].y;
            items[j].type=chests[i].item_type; items[j].respawn_timer=-1;
        }
    }
}

//###############################################
// CAMERA
//###############################################
void process_camera(double dt){
    double pm=cfg.px_per_m, sw=cfg.screen_w, sh=cfg.screen_h;
    double lerp=1.0-pow(0.02,dt);
    camera.x_px+=(player.base.x*pm-sw*0.5-camera.x_px)*lerp;
    camera.y_px+=(player.base.y*pm-sh*0.5-camera.y_px)*lerp;
}

//###############################################
// WIN CHECK
//###############################################
void process_win_check(){
    if(game_state==STATE_WIN) return;
    for(int i=0;i<decor_count_actual;i++){
        if(!decors[i].is_teleporter) continue;
        if(check_two_box_2d_hit_centralized(
            player.base.x,player.base.y,cfg.player_w,cfg.player_h,
            decors[i].x,decors[i].y,decors[i].w,decors[i].h)){
            game_state=STATE_WIN; return;
        }
    }
}

#endif
