#ifndef PROCESS_HELPER_H
#define PROCESS_HELPER_H
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include "10_data.h"
#include "external_lib/common_logic.h"
#include "external_lib/physic.h"

//###############################################
// HELPERS
//###############################################
static inline double dclamp(double v, double lo, double hi){
    return v < lo ? lo : v > hi ? hi : v;
}
static inline double dsign(double v){
    return v > 0 ? 1.0 : v < 0 ? -1.0 : 0.0;
}

//###############################################
// PLAYER MOVEMENT  (fix #1, #5)
//###############################################
void process_player_movement(double dt){
    int move_left  = kb.key_a.hold;
    int move_right = kb.key_d.hold;

    // --- GOD MODE: free fly, skip all physics ---
    if(player.god_mode){
        double fly_speed = 600.0;
        player.base.vx = 0; player.base.vy = 0; player.base.ay = 0;
        if(move_left)           player.base.x -= fly_speed * dt;
        if(move_right)          player.base.x += fly_speed * dt;
        if(kb.key_w.hold)       player.base.y -= fly_speed * dt;
        if(kb.key_s.hold)       player.base.y += fly_speed * dt;
        if(kb.key_space.hold)   player.base.y -= fly_speed * dt;
        // clamp to world
        if(player.base.x < 0)        player.base.x = 0;
        if(player.base.x > WORLD_W)  player.base.x = WORLD_W;
        if(player.base.y < 0)        player.base.y = 0;
        if(player.base.y > WORLD_H)  player.base.y = WORLD_H;
        return;
    }

    // --- EDGE GRAB LOGIC  (fix #1 and #4) ---
    // While grabbing: only allow jump to release, or pressing away from wall
    if(player.edge_grab){
        // pressing away from wall releases grab and lets player fall
        int release = 0;
        if(player.grab_wall_dir == -1 && move_right) release = 1; // left wall, press right = release
        if(player.grab_wall_dir ==  1 && move_left)  release = 1; // right wall, press left = release

        if(release){
            player.edge_grab = 0;
            player.grab_wall_dir = 0;
            // do NOT jump, just fall
        } else {
            // locked to wall: no horizontal movement, slow fall
            player.base.vx = 0;
            player.base.vy *= EDGE_GRAB_SLIDE;
            player.base.ay  = 0; // gravity off while grabbing
        }
    } else {
        player.base.ay = GRAVITY;
    }

    // --- DASH ---
    if(player.dashing > 0){
        player.dashing--;
        player.base.vx = player.dash_dir * DASH_SPEED;
        player.base.vy = 0;
        player.base.ay = 0;
    } else if(!player.edge_grab){
        // normal horizontal movement
        double target_vx = 0;
        if(move_left)  { target_vx = -MOVE_SPEED; player.last_move_dir = -1; }
        if(move_right) { target_vx =  MOVE_SPEED; player.last_move_dir =  1; }

        if(target_vx != 0){
            double diff       = target_vx - player.base.vx;
            double accel_step = MOVE_ACCEL * dt;
            if(fabs(diff) < accel_step) player.base.vx = target_vx;
            else player.base.vx += dsign(diff) * accel_step;
        } else {
            player.base.vx *= MOVE_FRICTION;
            if(fabs(player.base.vx) < 1.0) player.base.vx = 0;
        }
    }

    // --- JUMP ---
    if(kb.key_space.click){
        if(player.edge_grab){
            // wall jump
            player.edge_grab      = 0;
            player.grab_wall_dir  = 0;
            player.base.ay        = GRAVITY;
            double jvy = (player.jump_boost_timer > 0) ? JUMP_BOOST_VY : JUMP_VY;
            player.base.vy        = jvy;
            player.base.vx        = -player.on_wall * 300.0; // push away from wall
            player.jump_count     = 1;
            player.on_wall        = 0;
        } else if(player.on_ground || player.jump_count < 1){
            double jvy = (player.jump_boost_timer > 0) ? JUMP_BOOST_VY : JUMP_VY;
            player.base.vy    = jvy;   // fix #5: use constants correctly
            player.jump_count++;
            player.on_ground  = 0;
        }
    }

    // --- DASH TRIGGER ---
    int dash_key = kb.key_shift_l.click || kb.key_shift_r.click;
    if(dash_key && player.dash_ready && !player.dashing && !player.edge_grab){
        player.dashing    = DASH_FRAMES;
        player.dash_dir   = player.last_move_dir;
        player.dash_ready = 0;
    }

    update_base(&player.base, dt);

    // hard X clamp
    if(player.base.x < player.base.col_w/2)
        player.base.x = player.base.col_w/2;
    if(player.base.x > WORLD_W - player.base.col_w/2)
        player.base.x = WORLD_W - player.base.col_w/2;

    if(player.invincible       > 0) player.invincible--;
    if(player.jump_boost_timer > 0) player.jump_boost_timer--;
}

//###############################################
// TERRAIN COLLISION  (fix #2, #4)
//###############################################
void process_terrain_collision(){
    if(player.god_mode) return;
    player.on_ground = 0;
    player.on_wall   = 0;

    double px = player.base.x;
    double py = player.base.y;
    double pw = player.base.col_w;
    double ph = player.base.col_h;

    for(int i = 0; i < terrain_count_actual; i++){
        if(terrains[i].broken) continue;

        double tx = terrains[i].base.x;
        double ty = terrains[i].base.y;
        double tw = terrains[i].base.col_w;
        double th = terrains[i].base.col_h;

        if(!check_two_box_2d_hit_centralized(px, py, pw, ph, tx, ty, tw, th)) continue;

        double overlap_x = (pw + tw) / 2.0 - fabs(px - tx);
        double overlap_y = (ph + th) / 2.0 - fabs(py - ty);
        if(overlap_x <= 0 || overlap_y <= 0) continue;


        if(overlap_y < overlap_x){
            // vertical resolution
            if(py < ty){
                // landing on top
                player.base.y -= overlap_y;
                if(player.base.vy > 0){
                    if(terrains[i].type == TERRAIN_BREAK && terrains[i].hp > 0){
                        terrains[i].hp--;
                        if(terrains[i].hp <= 0){
                            terrains[i].broken       = 1;
                            terrains[i].broken_timer = 300;
                            player.base.vy = -250;
                        } else {
                            player.base.vy = 0;
                        }
                    } else {
                        player.base.vy = 0;
                    }
                    player.on_ground  = 1;
                    player.jump_count = 0;
                    player.dash_ready = 1;
                    player.edge_grab  = 0;
                    player.grab_wall_dir = 0;
                }
            } else {
                // hit from below
                player.base.y += overlap_y;
                if(player.base.vy < 0) player.base.vy = 0;
            }
            py = player.base.y;
        } else {
            // horizontal / wall resolution
            int wall_side;
            if(px < tx){
                player.base.x -= overlap_x;
                wall_side = 1;  // right wall of player touching left face of terrain
                if(player.base.vx > 0) player.base.vx = 0;
            } else {
                player.base.x += overlap_x;
                wall_side = -1; // left wall of player touching right face of terrain
                if(player.base.vx < 0) player.base.vx = 0;
            }
            player.on_wall = wall_side;
            px = player.base.x;

            // edge grab: ONLY if player is pressing INTO the wall (fix #4)
            // pressing left (key_a) while touching left wall (wall_side=-1)
            // pressing right (key_d) while touching right wall (wall_side=1)
            int pressing_into_wall =
                (wall_side == -1 && kb.key_a.hold) ||
                (wall_side ==  1 && kb.key_d.hold);

            if(!player.on_ground && pressing_into_wall && !player.edge_grab){
                player.edge_grab     = 1;
                player.grab_wall_dir = wall_side;
                player.base.vy = dclamp(player.base.vy, -200, 200);
            }
        }
    }

    // if no longer touching a wall, release grab
    if(player.on_wall == 0 && player.edge_grab){
        player.edge_grab     = 0;
        player.grab_wall_dir = 0;
    }
    if(player.on_ground) player.edge_grab = 0;
}

//###############################################
// BREAK TERRAIN RESPAWN
//###############################################
void process_terrain_timers(){
    for(int i = 0; i < terrain_count_actual; i++){
        if(!terrains[i].broken) continue;
        terrains[i].broken_timer--;
        if(terrains[i].broken_timer <= 0){
            terrains[i].broken = 0;
            terrains[i].hp     = 1;
        }
    }
}

//###############################################
// COINS
//###############################################
void process_coins(){
    double px = player.base.x;
    double py = player.base.y;
    for(int i = 0; i < COIN_COUNT; i++){
        if(coins[i].collected) continue;
        if(check_two_box_2d_hit_centralized(px,py,28,36,
                                            coins[i].x,coins[i].y,24,24)){
            coins[i].collected = 1;
            player.score++;
        }
    }
}

//###############################################
// ITEMS  (fix #5: boost properly applies JUMP_BOOST_VY)
//###############################################
void process_items(){
    double px = player.base.x;
    double py = player.base.y;
    for(int i = 0; i < ITEM_COUNT; i++){
        if(!items[i].active){
            items[i].respawn_timer--;
            if(items[i].respawn_timer <= 0) items[i].active = 1;
            continue;
        }
        if(check_two_box_2d_hit_centralized(px,py,28,36,
                                            items[i].x,items[i].y,24,24)){
            items[i].active        = 0;
            items[i].respawn_timer = 400;
            player.jump_boost_timer = 200;
        }
    }
}

//###############################################
// ENEMIES
//###############################################
void process_enemies(){
    double px = player.base.x;
    double py = player.base.y;

    for(int i = 0; i < ENEMY_COUNT; i++){
        struct enemy_data* e = &enemies[i];

        if(!e->active){
            if(e->type == ENEMY_BOSS && e->hp <= 0) continue;
            e->stun_timer--;
            if(e->stun_timer <= 0){
                e->active = 1;
                e->base.x = (e->patrol_x_min + e->patrol_x_max) / 2.0;
            }
            continue;
        }

        // patrol
        if(!e->dashing){
            double spd = 100.0 / 20.0;
            e->base.x += e->patrol_dir * spd;
            if(e->base.x >= e->patrol_x_max){ e->base.x = e->patrol_x_max; e->patrol_dir=-1; }
            if(e->base.x <= e->patrol_x_min){ e->base.x = e->patrol_x_min; e->patrol_dir= 1; }
        }

        // dash toward player
        e->dash_timer--;
        if(e->dash_timer <= 0 && !e->dashing){
            int freq = (e->type==ENEMY_BOSS) ? 50 : 100;
            e->dash_timer = freq - 20 + (rand() % 40);
            e->dashing    = 12;
            e->dash_vx    = (px > e->base.x)
                            ? fabs(e->dash_vx) : -fabs(e->dash_vx);
        }
        if(e->dashing > 0){
            e->base.x += e->dash_vx / 20.0;
            e->dashing--;
            e->base.x = dclamp(e->base.x, e->patrol_x_min, e->patrol_x_max);
        }

        e->base.y = e->patrol_y; // snap to terrain y

        // player collision
        double ew = e->base.col_w;
        double eh = e->base.col_h;
        if(!check_two_box_2d_hit_centralized(px,py,28,36,
                                             e->base.x,e->base.y,ew,eh)) continue;

        double player_bottom = py + 18;
        double enemy_top     = e->base.y - eh/2;

        if(player.base.vy > 0 && player_bottom <= enemy_top + 14){
            // stomp
            e->hp--;
            if(e->hp <= 0){
                e->active      = 0;
                e->stun_timer  = (e->type==ENEMY_BOSS) ? 99999 : 60;
            } else {
                e->active      = 0;
                e->stun_timer  = (e->type==ENEMY_BOSS) ? 80 : 60;
            }
            double jvy = (player.jump_boost_timer > 0) ? JUMP_BOOST_VY*0.7 : JUMP_VY*0.7;
            player.base.vy = jvy;
        } else if(player.invincible == 0){
            player.invincible = INVINCIBLE_FRAMES;
            double push_dir   = dsign(px - e->base.x);
            if(push_dir == 0) push_dir = 1;
            double force = (e->type==ENEMY_BOSS) ? 500.0 : 400.0;
            player.base.vx = push_dir * force;
            player.base.vy = (e->type==ENEMY_BOSS) ? -450.0 : -350.0;
        }
    }
}

//###############################################
// CAMERA  (fix #6: based on player world pos)
//###############################################
void process_camera(){
    double tx = player.base.x - SCREEN_W / 2.0;
    double ty = player.base.y - SCREEN_H / 2.0;
    camera.x += (tx - camera.x) * 0.15;
    camera.y += (ty - camera.y) * 0.15;
    camera.x = dclamp(camera.x, 0, WORLD_W - SCREEN_W);
    camera.y = dclamp(camera.y, 0, WORLD_H - SCREEN_H);
}

//###############################################
// WIN CHECK — teleporter box collision
// Box defined in 10_data.h as teleporter_x/y/w/h
// Player jumps INTO it (overlap) to trigger win
//###############################################
void process_win_check(){
    if(game_state == STATE_WIN) return;
    // teleporter box: centered at (1000, 260), size 80x80
    double bx=1000, by=260, bw=80, bh=80;
    if(check_two_box_2d_hit_centralized(
        player.base.x, player.base.y, 28, 36,
        bx, by, bw, bh)){
        game_state = STATE_WIN;
    }
}


#endif
