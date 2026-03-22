#ifndef PROCESS_H
#define PROCESS_H
#include <stdio.h>
#include <stdlib.h>

#include "10_data.h"
#include "41_process_helper.h"
#include "external_lib/time_util.h"

void process(){
    update_timer(&tps_timer);
    if(!tps_timer.at_tagret_time){
		wait_sec(tps_timer.tagret_time - tps_timer.time);
		return;
    }
    // while(!tps_timer.at_tagret_time){
    //   double wait_t = tps_timer.tagret_time - tps_timer.time;
    //   if(wait_t > 0) wait_sec(wait_t);
    //   update_timer(&tps_timer);
    // }
    double dt=tps_timer.dt_sec;
    if(game_state!=STATE_PLAY){ render_flag=1; return; }
    process_player_movement(dt);
    process_terrain_collision();
    process_pobjs(dt);
    process_terrain_timers();
    process_coins();
    process_items();
    process_chests();
    process_enemies(dt);
    process_projectiles(dt);
    process_camera(dt);
    process_win_check();
    render_flag=1;
}

#endif
