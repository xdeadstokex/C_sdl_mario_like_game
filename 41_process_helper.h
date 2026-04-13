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

// forward declarations — defined in 20_init.h
void reset_player();
void reload_world();
void reset_game();

static inline void tele_weather_boss(struct enemy_data* e);
void process_p1_vs_p2();
#define HIT_BOX(x,y,w,h,bx,by,bw,bh) \
    check_two_box_2d_hit_centralized((x),(y),(w),(h),(bx),(by),(bw),(bh))
#define IS_BOSS(t) ((t)==ENEMY_BOSS||(t)==ENEMY_WEATHER_BOSS)
#define OUT_WORLD(x,y) ((x)<0||(x)>cfg.world_w||(y)<0||(y)>cfg.world_h)
#define PROJ_SIZE  0.2
#define TPS_SCALE(n) ((n)*cfg.tps/20)

//###############################################
// PLAYER SENSORS
//###############################################
static inline void update_player_sensors(){
    double cx = player.base.x + player.base.col_ox;
    double cy = player.base.y + player.base.col_oy;
    double hw = player.base.col_w*0.5, hh = player.base.col_h*0.5;
    double lrw = player.base.col_w*SENSOR_LR_W_PCT, lrh = player.base.col_h*SENSOR_LR_H_PCT;
    double udw = player.base.col_w*SENSOR_UD_W_PCT, udh = player.base.col_h*SENSOR_UD_H_PCT;
    player.sensors.lx=cx-hw; player.sensors.ly=cy;    player.sensors.lw=lrw; player.sensors.lh=lrh;
    player.sensors.rx=cx+hw; player.sensors.ry=cy;    player.sensors.rw=lrw; player.sensors.rh=lrh;
    player.sensors.ux=cx;    player.sensors.uy=cy-hh; player.sensors.uw=udw; player.sensors.uh=udh;
    player.sensors.dx=cx;    player.sensors.dy=cy+hh; player.sensors.dw=udw; player.sensors.dh=udh;
}

static inline void clear_sensor_flags(){
    player.sensors.l_active=player.sensors.r_active=
    player.sensors.u_active=player.sensors.d_active=0;
}

static inline void sense_one(double tx,double ty,double tw,double th,
                              int* hd,int* hu,int* hl,int* hr){
    *hd=HIT_BOX(player.sensors.dx,player.sensors.dy,player.sensors.dw,player.sensors.dh,tx,ty,tw,th);
    *hu=HIT_BOX(player.sensors.ux,player.sensors.uy,player.sensors.uw,player.sensors.uh,tx,ty,tw,th);
    *hl=HIT_BOX(player.sensors.lx,player.sensors.ly,player.sensors.lw,player.sensors.lh,tx,ty,tw,th);
    *hr=HIT_BOX(player.sensors.rx,player.sensors.ry,player.sensors.rw,player.sensors.rh,tx,ty,tw,th);
    player.sensors.d_active|=*hd; player.sensors.u_active|=*hu;
    player.sensors.l_active|=*hl; player.sensors.r_active|=*hr;
}

//###############################################
// PLAYER STATE HELPERS
//###############################################
static inline double jump_vy(){
    return player.jump_boost_timer>0 ? cfg.jump_boost_vy : cfg.jump_vy;
}

static inline void player_land(){
    player.on_ground=1; player.jump_count=0;
    player.dash_ready=1; player.edge_grab=0; player.grab_wall_dir=0;
}

static inline void player_do_jump(){
    play_sound(&sfx.jump);
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
static inline void apply_edge_grab(double dt,int ml,int mr){
    int release=(player.grab_wall_dir==-1&&mr)||(player.grab_wall_dir==1&&ml);
    if(release||player.on_ground){
        player.edge_grab=0; player.grab_wall_dir=0; player.base.ay=cfg.gravity;
    } else {
        player.base.vx=0;
        player.base.vy*=pow(cfg.edge_grab_slide,dt*20.0);
        player.base.ay=0;
    }
}

static inline void apply_horizontal_move(double dt,int ml,int mr,double cur_speed){
    double tvx=0;
    if(ml){ tvx=-cur_speed; player.last_move_dir=-1; }
    if(mr){ tvx= cur_speed; player.last_move_dir= 1; }
    if(tvx!=0){
        double diff=tvx-player.base.vx, step=cfg.move_accel*dt;
        player.base.vx+=(fabs(diff)<step)?diff:dsign(diff)*step;
    } else {
        player.base.vx*=cfg.move_friction;
        if(fabs(player.base.vx)<0.01) player.base.vx=0;
    }
}

static inline void apply_jump(){
    if(!player.input_jump_click) return;
    player.input_jump_click=0;
    if(player.edge_grab)                          player_do_wall_jump();
    else if(player.on_ground||player.sensors.d_active) player_do_jump();
    else if(player.jump_count<1){ player.base.vy=jump_vy(); player.jump_count++; }
}

static inline void apply_dash(int ml,int mr){
    if(!(player.input_dash&&player.dash_ready&&!player.dashing&&!player.edge_grab)) return;
    player.input_dash=0;
    play_sound(&sfx.dash);
    player.dashing=cfg.dash_frames;
    player.dash_dir=player.last_move_dir;
    player.dash_ready=0;
}

static inline void apply_shoot(){
    if(!player.input_shoot||player.fireball_ammo<=0) return;
    player.input_shoot=0;
    play_sound(&sfx.fireball);
    player.fireball_ammo--;
    for(int i=0;i<PROJ_COUNT;i++){
        if(projectiles[i].active) continue;
        projectiles[i]=(struct projectile_data){
            .active=1, .x=player.base.x, .y=player.base.y,
            .vx=player.last_move_dir*12.0, .vy=0,
            .dir=player.last_move_dir, .type=0
        };
        break;
    }
}

//###############################################
// PLAYER MOVEMENT
//###############################################
void process_player_movement(double dt){
    int ml=player.input_move_left, mr=player.input_move_right;

    if(player.speed_boost_timer>0) player.speed_boost_timer--;
    if(player.slow_timer>0)        player.slow_timer--;

    double cur_speed=cfg.move_speed;
    if(player.speed_boost_timer>0) cur_speed*=1.5;
    if(player.slow_timer>0)        cur_speed*=0.5;

    if(player.input_reload_world&&game_state==STATE_PLAY){
        printf("[control] reloading world...\n");
        reload_world(); player.input_reload_world=0;
    }

    if(player.god_mode){
        player.base.vx=player.base.vy=player.base.ay=0;
        double fly=6.0;
        if(ml)                     player.base.x-=fly*dt;
        if(mr)                     player.base.x+=fly*dt;
        if(player.input_move_up)   player.base.y-=fly*dt;
        if(player.input_move_down) player.base.y+=fly*dt;
        if(player.input_jump_hold) player.base.y-=fly*dt;
        player.base.x=dclamp(player.base.x,0,cfg.world_w);
        player.base.y=dclamp(player.base.y,0,cfg.world_h);
        return;
    }

    if(player.edge_grab) apply_edge_grab(dt,ml,mr);
    else                 player.base.ay=cfg.gravity;

    if(player.dashing>0){
        player.dashing--;
        player.base.vx=player.dash_dir*cfg.dash_speed;
        player.base.vy=player.base.ay=0;
    } else if(!player.edge_grab){
        apply_horizontal_move(dt,ml,mr,cur_speed);
    }

    apply_jump();
    apply_shoot();

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
static inline void handle_terrain_break(int i,double ty,double th){
    double oy=(player.base.col_h+th)*0.5-fabs(player.base.y-ty);
    player.base.y-=oy;
    if(--terrains[i].hp<=0){
        terrains[i].broken=1;
        terrains[i].broken_timer=TPS_SCALE(300);
        player.base.vy=-2.5;
    } else { player.base.vy=0; }
    player_land();
    update_player_sensors();
}

static inline void handle_wall_contact(int hl,int hr,double ty,double th){
    int ws=hr?1:-1;
    player.on_wall=ws;
    int pressing=(ws==1&&player.input_move_right)||(ws==-1&&player.input_move_left);
    double terrain_top=ty-th*0.5;
    int beside=(player.base.y+player.base.col_oy)>=terrain_top;
    if(!player.on_ground&&pressing&&!player.edge_grab&&beside){
        player.edge_grab=1;
        player.grab_wall_dir=ws;
        player.base.vy=dclamp(player.base.vy,-2.0,2.0);
    }
}

//###############################################
// TERRAIN COLLISION
//###############################################
void process_terrain_collision(){
    if(player.god_mode) return;
    player.on_ground=player.on_wall=0;
    update_player_sensors();

    for(int i=0;i<terrain_count_actual;i++){
        if(terrains[i].broken) continue;
        double tx=terrains[i].base.x, ty=terrains[i].base.y;
        double tw=terrains[i].base.col_w, th=terrains[i].base.col_h;

        int hd,hu,hl,hr;
        sense_one(tx,ty,tw,th,&hd,&hu,&hl,&hr);
        if(!hd&&!hu&&!hl&&!hr) continue;

        if(hd&&terrains[i].type==TERRAIN_BREAK&&terrains[i].hp>0){
            handle_terrain_break(i,ty,th); continue;
        }

        double vx_before=player.base.vx;
        int hit=resolve_terrain_collision(&player.base,tx,ty,tw,th);
        if(!hit) continue;
        if(hit&0x1) player.base.vx=vx_before;
        update_player_sensors();

        if(hd) player_land();
        if(hl||hr) handle_wall_contact(hl,hr,ty,th);
    }

    if(player.on_ground)          player.edge_grab=0;
    if(player.sensors.u_active){  player.edge_grab=0; player.grab_wall_dir=0; }
}

//###############################################
// TERRAIN TIMERS
//###############################################
void process_terrain_timers(){
    for(int i=0;i<terrain_count_actual;i++){
        if(terrains[i].warning_timer>0){
            if(--terrains[i].warning_timer<=0){
                terrains[i].broken=1;
                terrains[i].broken_timer=TPS_SCALE(80);
            }
        }
        if(!terrains[i].broken) continue;
        if(--terrains[i].broken_timer<=0){ terrains[i].broken=0; terrains[i].hp=1; }
    }
}

//###############################################
// POBJ PHYSICS
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
    double bw=p->base.col_w, bh=p->base.col_h;
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

static inline void p2_vs_pobj(struct pobj_data* p){
    if(lan.role!=LAN_HOST||!lan.connected) return;
    struct player_data* p2=&lan.p2;
    double bx=p->base.x+p->base.col_ox, by=p->base.y+p->base.col_oy;
    double bw=p->base.col_w, bh=p->base.col_h;
    double pw=cfg.player_w, ph=cfg.player_h;
    double ox=pw*0.5+bw*0.5-fabs(p2->base.x-bx);
    double oy=ph*0.5+bh*0.5-fabs(p2->base.y-by);
    if(ox<=0||oy<=0) return;
    if(oy<ox){
        if(p2->base.y<by){ p2->base.y-=oy; if(p2->base.vy>0) p2->base.vy=0; p2->on_ground=1; p2->jump_count=0; p2->dash_ready=1; }
        else              { p2->base.y+=oy; if(p2->base.vy<0) p2->base.vy=0; }
    } else {
        if(p2->base.x<bx){ p2->base.x-=ox; p->base.x+=ox*0.5; }
        else              { p2->base.x+=ox; p->base.x-=ox*0.5; }
        p->base.vx=p2->base.vx;
    }
}

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
        p2_vs_pobj(p);
    }
}

//###############################################
// ENEMY HELPERS
//###############################################
static inline void spawn_projectile(struct enemy_data* e,double px,double speed){
    for(int k=0;k<PROJ_COUNT;k++){
        if(projectiles[k].active) continue;
        int dir=(px>e->base.x)?1:-1;
        projectiles[k]=(struct projectile_data){
            .active=1, .x=e->base.x, .y=e->base.y,
            .vx=dir*speed, .vy=0, .dir=dir, .type=PROJ_TYPE_ENEMY
        };
        break;
    }
}

static inline int mage_in_range(struct enemy_data* e,double px,double py){
    double dx=px-e->base.x, dy=py-e->base.y;
    double max_dx=dclamp(MAGE_RANGE_BASE+dy*MAGE_RANGE_SCALE_Y,MAGE_RANGE_MIN,MAGE_RANGE_MAX);
    return fabs(dx)<max_dx && dy>MAGE_DY_MIN && dy<MAGE_DY_MAX;
}

static inline int terrain_hit_player(struct terrain_data* t,double px,double py){
    double pfoot=py+cfg.player_h*0.5;
    double top=t->base.y-t->base.col_h*0.5;
    return fabs(px-t->base.x)<(t->base.col_w*0.5+cfg.player_w*0.5-TERRAIN_HIT_EPS_X)
        && (top-pfoot>=TERRAIN_HIT_Y_MIN)
        && (top-pfoot< TERRAIN_HIT_Y_MAX)
        && player.base.vy>=0;
}

static inline void enemy_update_patrol(struct enemy_data* e,double dt){
    e->base.x+=e->patrol_dir*PATROL_SPEED*dt;
    if(e->base.x>=e->patrol_x_max){ e->base.x=e->patrol_x_max; e->patrol_dir=-1; }
    else if(e->base.x<=e->patrol_x_min){ e->base.x=e->patrol_x_min; e->patrol_dir=1; }
}

static inline void enemy_update_dash(struct enemy_data* e,double dt,double px){
    const enemy_cfg_t* c=&ENEMY_CFG[e->type];

    if(--e->action_timer<=0&&!e->dashing){
        int tps=(int)(1.0/dt+0.5);
        int freq=(c->is_boss&&e->hp<=5&&c->dash_freq_lowhp)?c->dash_freq_lowhp:c->dash_freq;
        e->action_timer=(freq-DASH_FREQ_OFFSET+rand()%DASH_FREQ_RAND)*tps/20;
        e->dashing=tps*DASH_TIME/DASH_TIME_DIV;
        double vx=fabs(e->dash_vx);
        e->dash_vx=(px>e->base.x)?vx:-vx;
        if(c->is_boss) spawn_projectile(e,px,DASH_PROJ_SPEED_BOSS);
    }

    if(e->dashing<=0) return;
    double mult=(c->is_boss&&e->hp<=5&&c->dash_speed_mult_lowhp)?c->dash_speed_mult_lowhp:c->dash_speed_mult;
    e->base.x+=e->dash_vx*mult*dt;
    e->dashing--;
    e->base.x=dclamp(e->base.x,e->patrol_x_min,e->patrol_x_max);
}

static inline void tele_weather_boss(struct enemy_data* e){
    static const struct { double min,max,y; } p[WEATHER_PLAT_COUNT]={
        {1.3,4.0,117.5},{14.5,16.5,116.3},{9.2,12.2,118.0}
    };
    int cur=-1;
    for(int i=0;i<WEATHER_PLAT_COUNT;i++)
        if(fabs(e->patrol_x_min-p[i].min)<0.1){ cur=i; break; }
    int next;
    do{ next=rand()%WEATHER_PLAT_COUNT; } while(next==cur&&cur!=-1);

    e->patrol_x_min=p[next].min; e->patrol_x_max=p[next].max;
    double py=cfg.world_h-p[next].y-e->base.col_h;
    e->patrol_y=py+e->base.col_h*0.5;
    e->base.x=(e->patrol_x_min+e->patrol_x_max)*0.5;
    e->base.y=e->patrol_y;
    e->dash_vx=0.0;
    e->action_timer=TPS_SCALE(WEATHER_RESET_CD);
}

static inline void enemy_apply_hit(struct enemy_data* e){
    const enemy_cfg_t* c=&ENEMY_CFG[e->type];
    e->active=0;
    if(e->hp<=0){ e->stun_timer=STUN_TIME_DEAD; return; }
    int stun=(c->is_boss&&c->stun_time_boss)?c->stun_time_boss:c->stun_time;
    e->stun_timer=TPS_SCALE(stun);
    if(c->instant_recover){ e->active=1; e->stun_timer=0; }
    if(c->teleport_on_hit){ tele_weather_boss(e); e->active=1; e->stun_timer=0; }
}

static inline void enemy_stomped(struct enemy_data* e){
    play_sound(&sfx.hit);
    --e->hp;
    enemy_apply_hit(e);
}

static inline void player_hit_enemy(struct enemy_data* e,double px){
    if(player.invincible) return;
    const enemy_cfg_t* c=&ENEMY_CFG[e->type];
    player.invincible=cfg.invincible_frames;
    if(--player.hp<=0){ play_sound(&sfx.die); reset_game(); return; }
    play_sound(&sfx.hitted);
    double push=dsign(px-e->base.x); if(push==0) push=1;
    player.base.vx=push*c->push_force;
    player.base.vy=c->push_lift;
    if(c->push_slow_time) player.slow_timer=TPS_SCALE(c->push_slow_time);
}

static inline void p2_hit_enemy(struct enemy_data* e, double p2x){
    if(lan.p2.invincible) return;
    const enemy_cfg_t* c=&ENEMY_CFG[e->type];
    lan.p2.invincible=cfg.invincible_frames;
    if(--lan.p2.hp<=0){ play_sound(&sfx.die); /* p2 dies: respawn */ 
        lan.p2.base.x=player.respawn_x+1.0; lan.p2.base.y=player.respawn_y;
        lan.p2.base.vx=lan.p2.base.vy=0; lan.p2.hp=5; return; }
    play_sound(&sfx.hitted);
    double push=dsign(p2x-e->base.x); if(push==0) push=1;
    lan.p2.base.vx=push*c->push_force;
    lan.p2.base.vy=c->push_lift;
    if(c->push_slow_time) lan.p2.slow_timer=TPS_SCALE(c->push_slow_time);
}

static inline void enemy_vs_p2(struct enemy_data* e, double p2x, double p2y){
    double ex=e->base.x,ey=e->base.y,ew=e->base.col_w,eh=e->base.col_h;
    /* stomp check: p2 falling down onto enemy top */
    if(lan.p2.base.vy>0){
        double foot=p2y+cfg.player_h*0.5;
        double etop=ey-eh*0.5;
        if(fabs(p2x-ex)<(cfg.player_w*0.5+ew*0.5)*0.8 && foot>=etop && foot<etop+0.3){
            enemy_stomped(e);
            lan.p2.base.vy=-5.0;
            return;
        }
    }
    if(HIT_BOX(p2x,p2y,cfg.player_w,cfg.player_h,ex,ey,ew,eh))
        p2_hit_enemy(e,p2x);
}

static inline void enemy_vs_player(struct enemy_data* e,double px,double py){
    double ex=e->base.x,ey=e->base.y,ew=e->base.col_w,eh=e->base.col_h;
    if(player.base.vy>0&&
       HIT_BOX(player.sensors.dx,player.sensors.dy,player.sensors.dw,player.sensors.dh,ex,ey,ew,eh)){
        enemy_stomped(e);
        player.base.vy=jump_vy()*JUMP_BOUNCE_SCALE;
        return;
    }
    if(HIT_BOX(px,py,cfg.player_w,cfg.player_h,ex,ey,ew,eh))
        player_hit_enemy(e,px);
}

//###############################################
// ENEMIES
//###############################################
void process_enemies(double dt){
    double px=player.base.x, py=player.base.y;
    /* target: nearest player (or p1 if solo) */
    double tx=px, ty=py;
    if(lan.role==LAN_HOST&&lan.connected){
        double d1=(px-player.base.x)*(px-player.base.x)+(py-player.base.y)*(py-player.base.y);
        double p2x=lan.p2.base.x, p2y=lan.p2.base.y;
        double d2=(p2x-player.base.x)*(p2x-player.base.x)+(p2y-player.base.y)*(p2y-player.base.y);
        /* d1 is always 0 (self), so target is whichever is closer to enemy — recalc below per enemy */
        (void)d1; (void)d2;
    }

    for(int i=0;i<enemy_count_actual;i++){
        struct enemy_data* e=&enemies[i];
        const enemy_cfg_t* c=&ENEMY_CFG[e->type];

        /* pick nearest player as target for this enemy */
        tx=px; ty=py;
        if(lan.role==LAN_HOST&&lan.connected){
            double p2x=lan.p2.base.x, p2y=lan.p2.base.y;
            double d1=(px-e->base.x)*(px-e->base.x)+(py-e->base.y)*(py-e->base.y);
            double d2=(p2x-e->base.x)*(p2x-e->base.x)+(p2y-e->base.y)*(p2y-e->base.y);
            if(d2<d1){ tx=p2x; ty=p2y; }
        }

        if(e->hp<=0&&!IS_BOSS(e->type)){
            double ry=dclamp(e->base.y+REVIVE_OFFSET_Y,0.0,cfg.world_h-WORLD_EDGE_PAD);
            if(py>ry&&py>e->base.y){
                e->hp=c->max_hp; e->active=1; e->stun_timer=0;
                e->base.x=(e->patrol_x_min+e->patrol_x_max)*0.5;
            }
        }

        if(!e->active){
            if(IS_BOSS(e->type)&&e->hp<=0) continue;
            if(--e->stun_timer<=0&&e->hp>0) e->active=1;
            continue;
        }

        if(!e->dashing) enemy_update_patrol(e,dt);

        switch(e->type){
        case ENEMY_DASHER:
        case ENEMY_BOSS:
            enemy_update_dash(e,dt,tx);
            break;

        case ENEMY_SHOOTER:
            if(--e->action_timer<=0){
                e->action_timer=TPS_SCALE(c->shoot_cd);
                spawn_projectile(e,tx,c->proj_speed);
            }
            break;

        case ENEMY_SWORD:
            if(e->action_timer>0) e->action_timer--;
            if(e->action_timer<=0&&HIT_BOX(tx,ty,cfg.player_w,cfg.player_h,
                                            e->base.x,e->base.y,e->base.col_w*3,e->base.col_h)){
                e->action_timer=TPS_SCALE(c->action_cd);
                /* hit whichever player is in range */
                if(HIT_BOX(px,py,cfg.player_w,cfg.player_h,e->base.x,e->base.y,e->base.col_w*3,e->base.col_h))
                    player_hit_enemy(e,px);
                if(lan.role==LAN_HOST&&lan.connected){
                    double p2x=lan.p2.base.x,p2y=lan.p2.base.y;
                    if(HIT_BOX(p2x,p2y,cfg.player_w,cfg.player_h,e->base.x,e->base.y,e->base.col_w*3,e->base.col_h))
                        p2_hit_enemy(e,p2x);
                }
            }
            break;

        case ENEMY_MAGE:
            if(e->action_timer>0) e->action_timer--;
            if(e->action_timer<=0&&mage_in_range(e,tx,ty)){
                for(int k=0;k<terrain_count_actual;k++){
                    struct terrain_data* t=&terrains[k];
                    if(t->broken||t->warning_timer>0) continue;
                    if(t->base.col_w>10||t->base.col_h>10) continue;
                    if(terrain_hit_player(t,tx,ty)){
                        t->warning_timer=TPS_SCALE(c->mage_warn);
                        e->action_timer =TPS_SCALE(c->mage_cd);
                        break;
                    }
                }
            }
            break;

        case ENEMY_WEATHER_BOSS:
            if(--e->action_timer<=0)
                e->action_timer=TPS_SCALE(c->weather_cd_base+rand()%c->weather_cd_var);
            e->dash_vx=c->weather_dash_speed*((rand()%2)?1:-1); {
            double dx2=px-e->base.x, dy2=py-e->base.y;
            if(dx2*dx2+dy2*dy2<c->weather_push_range_sq)
                player.base.vx+=e->dash_vx*c->weather_push_scale*dt;
            if(lan.role==LAN_HOST&&lan.connected){
                double p2x=lan.p2.base.x,p2y=lan.p2.base.y;
                double dx3=p2x-e->base.x, dy3=p2y-e->base.y;
                if(dx3*dx3+dy3*dy3<c->weather_push_range_sq)
                    lan.p2.base.vx+=e->dash_vx*c->weather_push_scale*dt;
            }
            } break;
        }

        e->base.y=e->patrol_y;
        enemy_vs_player(e,px,py);
        if(lan.role==LAN_HOST&&lan.connected)
            enemy_vs_p2(e,lan.p2.base.x,lan.p2.base.y);
    }
}

//###############################################
// PROJECTILES
//###############################################
static inline int proj_hit_any_terrain(double x,double y){
    for(int j=0;j<terrain_count_actual;j++){
        if(terrains[j].broken) continue;
        if(HIT_BOX(x,y,PROJ_SIZE,PROJ_SIZE,
                   terrains[j].base.x,terrains[j].base.y,
                   terrains[j].base.col_w,terrains[j].base.col_h)) return 1;
    }
    return 0;
}

static inline void handle_player_hit(struct projectile_data* pr){
    if(HIT_BOX(pr->x,pr->y,PROJ_SIZE,PROJ_SIZE,player.base.x,player.base.y,cfg.player_w,cfg.player_h)){
        pr->active=0;
        if(!player.invincible){
            player.invincible=cfg.invincible_frames;
            if(--player.hp<=0){ play_sound(&sfx.die); reset_game(); return; }
            play_sound(&sfx.hitted);
            player.base.vx=pr->dir*5.0; player.base.vy=-3.0;
        }
        return;
    }
    if(lan.role==LAN_HOST&&lan.connected&&
       HIT_BOX(pr->x,pr->y,PROJ_SIZE,PROJ_SIZE,lan.p2.base.x,lan.p2.base.y,cfg.player_w,cfg.player_h)){
        pr->active=0;
        if(!lan.p2.invincible){
            lan.p2.invincible=cfg.invincible_frames;
            if(--lan.p2.hp<=0){ play_sound(&sfx.die);
                lan.p2.base.x=player.respawn_x+1.0; lan.p2.base.y=player.respawn_y;
                lan.p2.base.vx=lan.p2.base.vy=0; lan.p2.hp=5; return; }
            play_sound(&sfx.hitted);
            lan.p2.base.vx=pr->dir*5.0; lan.p2.base.vy=-3.0;
        }
    }
}

static inline void handle_enemy_hit(struct projectile_data* pr){
    for(int j=0;j<enemy_count_actual;j++){
        struct enemy_data* e=&enemies[j];
        if(!e->active) continue;
        if(!HIT_BOX(pr->x,pr->y,PROJ_SIZE,PROJ_SIZE,e->base.x,e->base.y,e->base.col_w,e->base.col_h)) continue;
        pr->active=0;
        if(--e->hp<=0) play_sound(&sfx.hit);
        enemy_apply_hit(e);
        break;
    }
}

void process_projectiles(double dt){
    for(int i=0;i<PROJ_COUNT;i++){
        struct projectile_data* pr=&projectiles[i];
        if(!pr->active) continue;
        pr->x+=pr->vx*dt; pr->y+=pr->vy*dt;
        if(OUT_WORLD(pr->x,pr->y)||proj_hit_any_terrain(pr->x,pr->y)){ pr->active=0; continue; }
        if(pr->type==PROJ_TYPE_ENEMY) handle_player_hit(pr);
        else                          handle_enemy_hit(pr);
    }
}

//###############################################
// COINS
//###############################################
void process_coins(){
    for(int i=0;i<coin_count_actual;i++){
        if(coins[i].collected) continue;
        if(HIT_BOX(player.base.x,player.base.y,cfg.player_w,cfg.player_h,
                   coins[i].x,coins[i].y,0.24,0.24)){
            coins[i].collected=1; player.score++;
            play_sound(&sfx.coin); continue;
        }
        if(lan.role==LAN_HOST&&lan.connected&&
           HIT_BOX(lan.p2.base.x,lan.p2.base.y,cfg.player_w,cfg.player_h,
                   coins[i].x,coins[i].y,0.24,0.24)){
            coins[i].collected=1; lan.p2.score++;
            play_sound(&sfx.coin);
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
        /* P1 pickup */
        if(HIT_BOX(player.base.x,player.base.y,cfg.player_w,cfg.player_h,
                    items[i].x,items[i].y,0.24,0.24)){
            play_sound(&sfx.buff);
            items[i].active=0;
            if(items[i].respawn_timer!=-1) items[i].respawn_timer=TPS_SCALE(400);
            if     (items[i].type==1) player.speed_boost_timer=TPS_SCALE(200);
            else if(items[i].type==2) player.jump_boost_timer =TPS_SCALE(200);
            else if(items[i].type==3) player.fireball_ammo   +=3;
            continue;
        }
        /* P2 pickup */
        if(lan.role==LAN_HOST&&lan.connected&&
           HIT_BOX(lan.p2.base.x,lan.p2.base.y,cfg.player_w,cfg.player_h,
                   items[i].x,items[i].y,0.24,0.24)){
            play_sound(&sfx.buff);
            items[i].active=0;
            if(items[i].respawn_timer!=-1) items[i].respawn_timer=TPS_SCALE(400);
            if     (items[i].type==1) lan.p2.speed_boost_timer=TPS_SCALE(200);
            else if(items[i].type==2) lan.p2.jump_boost_timer =TPS_SCALE(200);
            else if(items[i].type==3) lan.p2.fireball_ammo   +=3;
        }
    }
}

//###############################################
// CHESTS
//###############################################
void process_chests(){
    double px=player.base.x, py=player.base.y;
    double p2x=lan.p2.base.x, p2y=lan.p2.base.y;
    int p2_active=(lan.role==LAN_HOST&&lan.connected);
    for(int i=0;i<chest_count_actual;i++){
        if(chests[i].state==1){ chests[i].show_key=0; continue; }
        double dx=px-chests[i].x, dy=py-chests[i].y;
        int p1_near=(dx*dx+dy*dy<1.5*1.5);
        int p2_near=0;
        if(p2_active){ double dx2=p2x-chests[i].x,dy2=p2y-chests[i].y; p2_near=(dx2*dx2+dy2*dy2<1.5*1.5); }
        chests[i].show_key=(p1_near||p2_near);
        int interact=(p1_near&&player.input_interact)||(p2_near&&lan.p2.input_interact);
        if(!chests[i].show_key||!interact) continue;
        player.input_interact=0; lan.p2.input_interact=0;
        play_sound(&sfx.chest);
        chests[i].state=1; chests[i].show_key=0;
        int j=0;
        for(;j<item_count_actual;j++) if(!items[j].active) break;
        if(j==item_count_actual&&item_count_actual<ITEM_COUNT) item_count_actual++;
        if(j<ITEM_COUNT){
            items[j]=(struct item_data){
                .active=1, .x=chests[i].x-0.3, .y=chests[i].y,
                .type=chests[i].item_type, .respawn_timer=-1
            };
        }
    }
}

//###############################################
// CAMERA
//###############################################
void process_camera(double dt){
    double pm=cfg.px_per_m, lerp=1.0-pow(0.02,dt);
    camera.x_px+=(player.base.x*pm-cfg.screen_w*0.5-camera.x_px)*lerp;
    camera.y_px+=(player.base.y*pm-cfg.screen_h*0.5-camera.y_px)*lerp;
}

//###############################################
// WIN CHECK
//###############################################
void process_win_check(){
    if(game_state==STATE_WIN) return;
    for(int i=0;i<decor_count_actual;i++){
        if(!decors[i].is_teleporter) continue;
        if(HIT_BOX(player.base.x,player.base.y,cfg.player_w,cfg.player_h,
                   decors[i].x,decors[i].y,decors[i].w,decors[i].h)){
            play_sound(&sfx.win); game_state=STATE_WIN; return;
        }
        if(lan.role==LAN_HOST&&lan.connected&&
           HIT_BOX(lan.p2.base.x,lan.p2.base.y,cfg.player_w,cfg.player_h,
                   decors[i].x,decors[i].y,decors[i].w,decors[i].h)){
            play_sound(&sfx.win); game_state=STATE_WIN; return;
        }
    }
}

//###############################################
// P1 vs P2 INTERACTION  (host only)
// - Stomp: falling player lands on other's head -> bounce
// - Side push: equal-mass AABB separation
//###############################################
void process_p1_vs_p2(){
    if(lan.role!=LAN_HOST||!lan.connected) return;
    double ax=player.base.x, ay=player.base.y;
    double bx=lan.p2.base.x, by=lan.p2.base.y;
    double pw=cfg.player_w, ph=cfg.player_h;
    double ow=pw-fabs(ax-bx), oh=ph-fabs(ay-by);
    if(ow<=0||oh<=0) return;

    // Stomp check: one player falling, foot near other's head
    // Resolve vertically if overlap is vertical-dominant
    if(oh < ow){
        // P1 stomping P2
        if(ay < by && player.base.vy > 0){
            player.base.y -= oh; player.base.vy = -fabs(player.base.vy)*0.6;
            player_land();
        // P2 stomping P1
        } else if(by < ay && lan.p2.base.vy > 0){
            lan.p2.base.y -= oh; lan.p2.base.vy = -fabs(lan.p2.base.vy)*0.6;
            lan.p2.on_ground=1; lan.p2.jump_count=0; lan.p2.dash_ready=1;
        } else {
            // ceiling case
            double half=oh*0.5;
            if(ay<by){ player.base.y-=half; lan.p2.base.y+=half; }
            else      { player.base.y+=half; lan.p2.base.y-=half; }
        }
    } else {
        // side push — equal share
        double half=ow*0.5;
        if(ax<bx){ player.base.x-=half; lan.p2.base.x+=half; }
        else      { player.base.x+=half; lan.p2.base.x-=half; }
        double avg=(player.base.vx+lan.p2.base.vx)*0.5;
        player.base.vx=avg; lan.p2.base.vx=avg;
    }
}

#endif