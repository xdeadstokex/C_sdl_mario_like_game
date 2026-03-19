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

//###############################################
// PLAYER MOVEMENT  (all units: meters, m/s)
//###############################################
void process_player_movement(double dt){
    int ml=kb.key_a.hold, mr=kb.key_d.hold;

    if(player.speed_boost_timer > 0) player.speed_boost_timer--;
    double cur_speed = (player.speed_boost_timer > 0) ? cfg.move_speed * 1.5 : cfg.move_speed;

    // GOD MODE
    if(player.god_mode){
        double fly=6.0;
        player.base.vx=0; player.base.vy=0; player.base.ay=0;
        if(ml)                player.base.x -= fly*dt;
        if(mr)                player.base.x += fly*dt;
        if(kb.key_w.hold)     player.base.y -= fly*dt;
        if(kb.key_s.hold)     player.base.y += fly*dt;
        if(kb.key_space.hold) player.base.y -= fly*dt;
        player.base.x=dclamp(player.base.x,0,cfg.world_w);
        player.base.y=dclamp(player.base.y,0,cfg.world_h);
        return;
    }

    // EDGE GRAB
    if(player.edge_grab){
        int release=(player.grab_wall_dir==-1&&mr)||(player.grab_wall_dir==1&&ml);
        if(release){ player.edge_grab=0; player.grab_wall_dir=0; }
        else { player.base.vx=0; player.base.vy*=cfg.edge_grab_slide; player.base.ay=0; }
    } else {
        player.base.ay=cfg.gravity;
    }

    // DASH
    if(player.dashing>0){
        player.dashing--;
        player.base.vx=player.dash_dir*cfg.dash_speed;
        player.base.vy=0; player.base.ay=0;
    } else if(!player.edge_grab){
        double tvx=0;
        if(ml){ tvx=-cur_speed; player.last_move_dir=-1; }
        if(mr){ tvx= cur_speed; player.last_move_dir= 1; }
        if(tvx!=0){
            double diff=tvx-player.base.vx;
            double step=cfg.move_accel*dt;
            if(fabs(diff)<step) player.base.vx=tvx;
            else player.base.vx+=dsign(diff)*step;
        } else {
            player.base.vx*=cfg.move_friction;
            if(fabs(player.base.vx)<0.01) player.base.vx=0;
        }
    }

    // JUMP
    if(kb.key_space.click){
        if(player.edge_grab){
            player.edge_grab=0; player.grab_wall_dir=0;
            player.base.ay=cfg.gravity;
            double jvy=(player.jump_boost_timer>0)?cfg.jump_boost_vy:cfg.jump_vy;
            player.base.vy=jvy;
            player.base.vx=-player.on_wall*3.0;
            player.jump_count=1; player.on_wall=0;
        } else if(player.on_ground||player.jump_count<1){
            double jvy=(player.jump_boost_timer>0)?cfg.jump_boost_vy:cfg.jump_vy;
            player.base.vy=jvy;
            player.jump_count++;
            player.on_ground=0;
        }
    }

    // DASH TRIGGER
    if((kb.key_shift_l.click||kb.key_shift_r.click)
       &&player.dash_ready&&!player.dashing&&!player.edge_grab){
        player.dashing=cfg.dash_frames;
        player.dash_dir=player.last_move_dir;
        player.dash_ready=0;
    }

    update_base(&player.base,dt);

    // velocity caps
    if(player.base.vx >  cfg.max_vx) player.base.vx =  cfg.max_vx;
    if(player.base.vx < -cfg.max_vx) player.base.vx = -cfg.max_vx;
    if(player.base.vy >  cfg.max_vy) player.base.vy =  cfg.max_vy;
    if(player.base.vy < -cfg.max_vy) player.base.vy = -cfg.max_vy;

    // X world clamp
    if(player.base.x < cfg.player_w/2)             player.base.x = cfg.player_w/2;
    if(player.base.x > cfg.world_w-cfg.player_w/2) player.base.x = cfg.world_w-cfg.player_w/2;

    if(player.invincible>0)       player.invincible--;
    if(player.jump_boost_timer>0) player.jump_boost_timer--;
}

//###############################################
// TERRAIN COLLISION  (player)
// Generic push-out via resolve_terrain_collision.
// Player-specific flags (on_ground, edge_grab,
// TERRAIN_BREAK) handled here after the call.
//###############################################
void process_terrain_collision(){
    if(player.god_mode) return;
    player.on_ground=0; player.on_wall=0;

    for(int i=0;i<terrain_count_actual;i++){
        if(terrains[i].broken) continue;

        double tx=terrains[i].base.x, ty=terrains[i].base.y;
        double tw=terrains[i].base.col_w, th=terrains[i].base.col_h;

        // peek overlap before resolving — needed for TERRAIN_BREAK and edge grab
        double bx=player.base.x, by=player.base.y;
        double bw=player.base.col_w, bh=player.base.col_h;
        double ox=(bw+tw)/2.0-fabs(bx-tx);
        double oy=(bh+th)/2.0-fabs(by-ty);
        int vertical_hit = (ox>0&&oy>0) && (oy<ox);
        int landing      = vertical_hit && (by<ty) && (player.base.vy>0);

        // TERRAIN_BREAK: intercept before generic resolver zeroes vy
        if(landing && terrains[i].type==TERRAIN_BREAK && terrains[i].hp>0){
            player.base.y -= oy;
            terrains[i].hp--;
            if(terrains[i].hp<=0){
                terrains[i].broken=1;
                terrains[i].broken_timer=300;
                player.base.vy=-2.5;
            } else {
                player.base.vy=0;
            }
            player.on_ground=1; player.jump_count=0;
            player.dash_ready=1; player.edge_grab=0; player.grab_wall_dir=0;
            continue;
        }

        int hit = resolve_terrain_collision(&player.base, tx, ty, tw, th);
        if(!hit) continue;

        if(hit & 0x1){
            // vertical — floor landing
            if(player.base.vy >= 0){
                player.on_ground=1; player.jump_count=0;
                player.dash_ready=1; player.edge_grab=0; player.grab_wall_dir=0;
            }
        }
        if(hit & 0x2){
            // horizontal — wall, check edge grab
            int ws = (bx < tx) ? 1 : -1;
            player.on_wall = ws;
            int pressing = (ws==-1&&kb.key_a.hold)||(ws==1&&kb.key_d.hold);
            if(!player.on_ground && pressing && !player.edge_grab){
                player.edge_grab=1; player.grab_wall_dir=ws;
                player.base.vy=dclamp(player.base.vy,-2.0,2.0);
            }
        }
    }

    if(player.on_wall==0 && player.edge_grab){ player.edge_grab=0; player.grab_wall_dir=0; }
    if(player.on_ground) player.edge_grab=0;
}

//###############################################
// TERRAIN TIMERS
//###############################################
void process_terrain_timers(){
    for(int i=0;i<terrain_count_actual;i++){
        if(!terrains[i].broken) continue;
        if(--terrains[i].broken_timer<=0){
            terrains[i].broken=0; terrains[i].hp=1;
        }
    }
}

//###############################################
// POBJS  (pushable physics objects)
// 1. gravity + integrate
// 2. terrain push-out (generic)
// 3. player <-> pobj two-way collision
//###############################################
void process_pobjs(double dt){
    for(int i=0;i<pobj_count_actual;i++){
        struct pobj_data* p=&pobjs[i];
        if(!p->active) continue;

        // gravity
        p->base.ay = cfg.gravity;
        update_base(&p->base, dt);

        // velocity caps (reuse player caps — reasonable for any obj)
        if(p->base.vx >  cfg.max_vx) p->base.vx =  cfg.max_vx;
        if(p->base.vx < -cfg.max_vx) p->base.vx = -cfg.max_vx;
        if(p->base.vy >  cfg.max_vy) p->base.vy =  cfg.max_vy;
        if(p->base.vy < -cfg.max_vy) p->base.vy = -cfg.max_vy;

        // terrain collision
        p->on_ground = 0;
        for(int j=0;j<terrain_count_actual;j++){
            if(terrains[j].broken) continue;
            int hit = resolve_terrain_collision(&p->base,
                terrains[j].base.x, terrains[j].base.y,
                terrains[j].base.col_w, terrains[j].base.col_h);
            if(hit & 0x1) p->on_ground = 1;
        }

        // player <-> pobj: horizontal side push only
        {
            double ax = player.base.x + player.base.col_ox;
            double ay = player.base.y + player.base.col_oy;
            double aw = player.base.col_w, ah = player.base.col_h;
            double bx = p->base.x + p->base.col_ox;
            double by = p->base.y + p->base.col_oy;
            double bw = p->base.col_w, bh = p->base.col_h;

            double ov_x = (aw + bw) / 2.0 - fabs(ax - bx);
            double ov_y = (ah + bh) / 2.0 - fabs(ay - by);

            if(ov_x > 0 && ov_y > 0){
                if(ov_x < ov_y){
                    // side hit
                    if(ax < bx){ player.base.x -= ov_x; p->base.x += ov_x; }
                    else        { player.base.x += ov_x; p->base.x -= ov_x; }
                    p->base.vx = player.base.vx;
                } else {
                    // vertical hit — push player only
                    if(ay < by){ player.base.y -= ov_y; if(player.base.vy > 0){ player.base.vy = 0; player.on_ground = 1; player.jump_count = 0; player.dash_ready = 1; } }
                    else       { player.base.y += ov_y; if(player.base.vy < 0)  player.base.vy = 0; }
                }
            }
        }
    }
}

//###############################################
// COINS
//###############################################
void process_coins(){
    double px=player.base.x, py=player.base.y;
    double pw=cfg.player_w, ph=cfg.player_h;
    for(int i=0;i<coin_count_actual;i++){
        if(coins[i].collected) continue;
        if(check_two_box_2d_hit_centralized(px,py,pw,ph,coins[i].x,coins[i].y,0.24,0.24)){
            coins[i].collected=1; player.score++;
        }
    }
}

//###############################################
// ITEMS
//###############################################
void process_items(){
    double px=player.base.x, py=player.base.y;
    double pw=cfg.player_w, ph=cfg.player_h;
    for(int i=0;i<item_count_actual;i++){
        if(!items[i].active){
            if(--items[i].respawn_timer > 0){
                if(--items[i].respawn_timer == 0){
                    items[i].active = 1;
                }
            }
            continue;
        }
        if(check_two_box_2d_hit_centralized(px,py,pw,ph,items[i].x,items[i].y,0.24,0.24)){
            items[i].active=0;
            if(items[i].respawn_timer != -1) items[i].respawn_timer=400;
            if(items[i].type == 1) player.speed_boost_timer = 200;
            else                   player.jump_boost_timer  = 200;
        }
    }
}

//###############################################
// ENEMIES
//###############################################
void process_enemies(){
    double px=player.base.x, py=player.base.y;
    for(int i=0;i<enemy_count_actual;i++){
        struct enemy_data* e=&enemies[i];
        if(!e->active){
            if(e->type==ENEMY_BOSS&&e->hp<=0) continue;
            if(--e->stun_timer<=0){
                e->active=1;
                e->base.x=(e->patrol_x_min+e->patrol_x_max)/2.0;
            }
            continue;
        }
        if(!e->dashing){
            double spd=1.0/20.0;
            e->base.x+=e->patrol_dir*spd;
            if(e->base.x>=e->patrol_x_max){e->base.x=e->patrol_x_max;e->patrol_dir=-1;}
            if(e->base.x<=e->patrol_x_min){e->base.x=e->patrol_x_min;e->patrol_dir= 1;}
        }
        if(--e->dash_timer<=0&&!e->dashing){
            int freq=(e->type==ENEMY_BOSS)?50:100;
            e->dash_timer=freq-20+(rand()%40);
            e->dashing=12;
            e->dash_vx=(px>e->base.x)?fabs(e->dash_vx):-fabs(e->dash_vx);
        }
        if(e->dashing>0){
            e->base.x+=e->dash_vx/20.0;
            e->dashing--;
            e->base.x=dclamp(e->base.x,e->patrol_x_min,e->patrol_x_max);
        }
        e->base.y=e->patrol_y;

        double ew=e->base.col_w, eh=e->base.col_h;
        if(!check_two_box_2d_hit_centralized(px,py,cfg.player_w,cfg.player_h,
                                              e->base.x,e->base.y,ew,eh)) continue;
        double pb=py+cfg.player_h/2.0;
        double et=e->base.y-eh/2.0;
        if(player.base.vy>0&&pb<=et+0.14){
            if(--e->hp<=0){ e->active=0; e->stun_timer=(e->type==ENEMY_BOSS)?99999:60; }
            else           { e->active=0; e->stun_timer=(e->type==ENEMY_BOSS)?80:60; }
            double jvy=(player.jump_boost_timer>0)?cfg.jump_boost_vy*0.7:cfg.jump_vy*0.7;
            player.base.vy=jvy;
        } else if(player.invincible==0){
            player.invincible=cfg.invincible_frames;
            double push=dsign(px-e->base.x); if(push==0) push=1;
            double force=(e->type==ENEMY_BOSS)?5.0:4.0;
            player.base.vx=push*force;
            player.base.vy=(e->type==ENEMY_BOSS)?-4.5:-3.5;
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
        if(dx*dx+dy*dy < 1.5*1.5){
            chests[i].show_key=1;
            if(kb.key_e.click){
                chests[i].state=1; chests[i].show_key=0;
                int spawned=0;
                for(int j=0;j<item_count_actual;j++){
                    if(!items[j].active){
                        items[j].active=1;
                        items[j].x=chests[i].x-0.3; items[j].y=chests[i].y;
                        items[j].type=chests[i].item_type; items[j].respawn_timer=-1;
                        spawned=1; break;
                    }
                }
                if(!spawned && item_count_actual<ITEM_COUNT){
                    int j=item_count_actual;
                    items[j].active=1;
                    items[j].x=chests[i].x-0.3; items[j].y=chests[i].y;
                    items[j].type=chests[i].item_type; items[j].respawn_timer=-1;
                    item_count_actual++;
                }
            }
        } else {
            chests[i].show_key=0;
        }
    }
}

//###############################################
// CAMERA  (pixels, lerp follow)
//###############################################
void process_camera(){
    double pm=cfg.px_per_m, sw=cfg.screen_w, sh=cfg.screen_h;
    double tx=player.base.x*pm - sw/2.0;
    double ty=player.base.y*pm - sh/2.0;
    camera.x_px+=(tx-camera.x_px)*0.15;
    camera.y_px+=(ty-camera.y_px)*0.15;
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
