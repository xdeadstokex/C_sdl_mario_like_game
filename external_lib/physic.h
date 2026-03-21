#ifndef PHYSIC_H
#define PHYSIC_H

#include "common_logic.h"
#include <math.h>

//###############################################
//####           STRUCT                      ####
//###############################################

struct physic_base_data {
    double x, y;
    double vx, vy;
    double ax, ay;
    double weight;       // kg, negative = immovable
    double friction;     // 0.0 = ice, 1.0 = full stop
    double restitution;  // 0.0 = no bounce, 1.0 = full bounce
    // hitbox offset from center
    double col_ox, col_oy;
    double col_w, col_h;
    int colliding;
};


//###############################################
//####           INIT                        ####
//###############################################

static inline void set_physic_base(struct physic_base_data* base,
    double x, double y,
    double vx, double vy,
    double ax, double ay,
    double weight,
    double friction, double restitution,
    double col_ox, double col_oy,
    double col_w, double col_h){
    base->x = x; base->y = y;
    base->vx = vx; base->vy = vy;
    base->ax = ax; base->aay = ay;
    base->weight = weight;
    base->friction = friction;
    base->restitution = restitution;
    base->col_ox = col_ox; base->col_oy = col_oy;
    base->col_w = col_w;   base->col_h = col_h;
    base->colliding = 0;
}


//###############################################
//####           UPDATE                      ####
//###############################################

static inline void update_base(struct physic_base_data* base, double dt_sec){
    base->vx += base->ax * dt_sec;
    base->vy += base->ay * dt_sec;
    base->x  += base->vx * dt_sec;
    base->y  += base->vy * dt_sec;
}


//###############################################
//####           COLLISION                   ####
//###############################################

static inline int check_entity_collision(struct physic_base_data* a, struct physic_base_data* b){
    double ax = a->x + a->col_ox;
    double ay = a->y + a->col_oy;
    double bx = b->x + b->col_ox;
    double by = b->y + b->col_oy;

    if(!check_two_box_2d_hit_centralized(ax, ay, a->col_w, a->col_h,
                                         bx, by, b->col_w, b->col_h)){
        a->colliding = 0;
        b->colliding = 0;
        return 0;
    }

    double overlap_x = (a->col_w + b->col_w) / 2.0 - fabs(ax - bx);
    double overlap_y = (a->col_h + b->col_h) / 2.0 - fabs(ay - by);

    double push_x = 0, push_y = 0;
    if(overlap_x < overlap_y){
        push_x = (ax < bx) ? -overlap_x : overlap_x;
    } else {
        push_y = (ay < by) ? -overlap_y : overlap_y;
    }

    int a_moves = a->weight >= 0;
    int b_moves = b->weight >= 0;

    // position push
    if(a_moves && b_moves){
        double total = a->weight + b->weight;
        double ratio = (total > 0) ? b->weight / total : 0.5;
        a->x += push_x * ratio;
        a->y += push_y * ratio;
        b->x -= push_x * (1.0 - ratio);
        b->y -= push_y * (1.0 - ratio);
    } else if(a_moves){
        a->x += push_x;
        a->y += push_y;
    } else if(b_moves){
        b->x -= push_x;
        b->y -= push_y;
    }

    // velocity transfer once on first contact
    if(a->colliding == 0 && b->colliding == 0){
        double rest = (a->restitution + b->restitution) / 2.0;

        if(a_moves && b_moves){
            double total = a->weight + b->weight;
            double new_vx_a = (a->vx * (a->weight - b->weight) + 2 * b->weight * b->vx) / total;
            double new_vy_a = (a->vy * (a->weight - b->weight) + 2 * b->weight * b->vy) / total;
            double new_vx_b = (b->vx * (b->weight - a->weight) + 2 * a->weight * a->vx) / total;
            double new_vy_b = (b->vy * (b->weight - a->weight) + 2 * a->weight * a->vy) / total;
            a->vx = new_vx_a * rest; a->vy = new_vy_a * rest;
            b->vx = new_vx_b * rest; b->vy = new_vy_b * rest;
        } else if(a_moves){
            if(push_x != 0) a->vx = -a->vx * rest;
            if(push_y != 0) a->vy = -a->vy * rest;
        } else if(b_moves){
            if(push_x != 0) b->vx = -b->vx * rest;
            if(push_y != 0) b->vy = -b->vy * rest;
        }

        a->colliding = 1;
        b->colliding = 1;
    }

    // friction perpendicular to contact, every frame while touching
    double avg_friction = (a->friction + b->friction) / 2.0;
    if(push_y != 0){  // floor/ceiling -> slow horizontal
        if(a_moves) a->vx *= (1.0 - avg_friction);
        if(b_moves) b->vx *= (1.0 - avg_friction);
    }
    if(push_x != 0){  // wall -> slow vertical (edge grab)
        if(a_moves) a->vy *= (1.0 - avg_friction);
        if(b_moves) b->vy *= (1.0 - avg_friction);
    }

    return 1;
}


//###############################################
//####     GENERIC TERRAIN PUSH-OUT          ####
//###############################################
// Resolves one body against one static terrain rect.
// Returns bitmask: 0x1 = vertical hit (floor/ceil), 0x2 = horizontal hit (wall)
// Caller is responsible for on_ground / on_wall flags using the return value.
//
// Terrain is treated as immovable (weight < 0 equivalent).
// body->friction and body->restitution are used for response.

static inline int resolve_terrain_collision(
    struct physic_base_data* body,
    double tx, double ty, double tw, double th)
{
    double bx = body->x + body->col_ox;
    double by = body->y + body->col_oy;
    double bw = body->col_w, bh = body->col_h;

    if(!check_two_box_2d_hit_centralized(bx, by, bw, bh, tx, ty, tw, th))
        return 0;

    double ox = (bw + tw) / 2.0 - fabs(bx - tx);
    double oy = (bh + th) / 2.0 - fabs(by - ty);
    if(ox <= 0 || oy <= 0) return 0;

    if(oy < ox){
        // vertical hit (floor or ceiling)
        if(by < ty){
            body->y -= oy;
            if(body->vy > 0){
                body->vy = -body->vy * body->restitution;
                if(fabs(body->vy) < 0.05) body->vy = 0;
            }
        } else {
            body->y += oy;
            if(body->vy < 0){
                body->vy = -body->vy * body->restitution;
            }
        }
        body->vx *= (1.0 - body->friction);
        return 0x1;
    } else {
        // horizontal hit (wall)
        if(bx < tx) body->x -= ox;
        else        body->x += ox;
        if(body->vx != 0){
            body->vx = -body->vx * body->restitution;
            if(fabs(body->vx) < 0.05) body->vx = 0;
        }
        body->vy *= (1.0 - body->friction);
        return 0x2;
    }
}

#endif