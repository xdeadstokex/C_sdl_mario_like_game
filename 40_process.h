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
    double dt=tps_timer.dt_sec;

    // -----------------------------------------------
    // CLIENT — send input, receive state, skip sim
    // -----------------------------------------------
    if(lan.role == LAN_CLIENT){
        if(game_state == STATE_NET_LOBBY || game_state == STATE_PLAY || game_state == STATE_WIN){ lan_client_tick(&lan); }
        if(game_state == STATE_PLAY)
            process_camera(dt);   // client drives its own camera (player = client's char after unpack)
        render_flag=1;
        return;
    }

    // -----------------------------------------------
    // LOBBY / non-play states — nothing to simulate
    // BUT host must still poll socket to receive MSG_JOIN
    // -----------------------------------------------
    if(game_state != STATE_PLAY){
        if(lan.role == LAN_HOST && lan.sock_open)
            lan_host_tick(&lan);
        render_flag=1;
        return;
    }

    // -----------------------------------------------
    // P1 simulation (always)
    // -----------------------------------------------
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

    // -----------------------------------------------
    // HOST — run P2 physics via global swap, broadcast
    // -----------------------------------------------
    if(lan.role == LAN_HOST && lan.connected){
        struct player_data tmp = player;
        player = lan.p2;
        process_player_movement(dt);
        process_terrain_collision();   // operates on global player (== p2 during swap)
        lan.p2 = player;
        player = tmp;

        process_p1_vs_p2();            // push P1 and P2 apart if overlapping

        lan_host_tick(&lan);
    }

    render_flag=1;
}

#endif