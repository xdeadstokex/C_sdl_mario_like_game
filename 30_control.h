#ifndef CONTROL_H
#define CONTROL_H
#include <stdio.h>
#include <stdlib.h>

#include "10_data.h"
#include "20_init.h"
#include "external_lib/sdl2_wrapper.h"
#include "external_lib/io_logic.h"

int control(){
    int mouse_motion      = 0;
    int mouse_click       = 0;
    int mouse_hold_left   = mouse.left.hold;
    int mouse_hold_right  = mouse.right.hold;
    int mouse_hold_middle = mouse.middle.hold;

    for(int start=0; get_next_event(&window);){
        if(!start){
            mouse_hold_left=0; mouse_hold_right=0; mouse_hold_middle=0;
            start=1;
        }
        if(window.quit)               return 0;
        if(window.mouse_click)        mouse_click       = 1;
        if(window.mouse_click_left)   mouse_hold_left   = 1;
        if(window.mouse_click_right)  mouse_hold_right  = 1;
        if(window.mouse_click_middle) mouse_hold_middle = 1;
        if(window.mouse_motion)       mouse_motion      = 1;
        if(window.resize_signal){
            cfg.screen_w = (double)window.w;
            cfg.screen_h = (double)window.h;
			init_menu_rects(&gui, cfg.screen_w, cfg.screen_h);
        }
    }

    mouse.motion = mouse_motion;
    move_mouse(&mouse, window.mouse_x, window.mouse_y);
    mouse.click = mouse_click;
    update_signal(&mouse.left,   mouse_hold_left);
    update_signal(&mouse.right,  mouse_hold_right);
    update_signal(&mouse.middle, mouse_hold_middle);

    kb.key_signal = window.key_signal;
    update_signal(&kb.key_a, window.key_a); update_signal(&kb.key_b, window.key_b);
    update_signal(&kb.key_c, window.key_c); update_signal(&kb.key_d, window.key_d);
    update_signal(&kb.key_e, window.key_e); update_signal(&kb.key_f, window.key_f);
    update_signal(&kb.key_g, window.key_g); update_signal(&kb.key_h, window.key_h);
    update_signal(&kb.key_i, window.key_i); update_signal(&kb.key_j, window.key_j);
    update_signal(&kb.key_k, window.key_k); update_signal(&kb.key_l, window.key_l);
    update_signal(&kb.key_m, window.key_m); update_signal(&kb.key_n, window.key_n);
    update_signal(&kb.key_o, window.key_o); update_signal(&kb.key_p, window.key_p);
    update_signal(&kb.key_q, window.key_q); update_signal(&kb.key_r, window.key_r);
    update_signal(&kb.key_s, window.key_s); update_signal(&kb.key_t, window.key_t);
    update_signal(&kb.key_u, window.key_u); update_signal(&kb.key_v, window.key_v);
    update_signal(&kb.key_w, window.key_w); update_signal(&kb.key_x, window.key_x);
    update_signal(&kb.key_y, window.key_y); update_signal(&kb.key_z, window.key_z);
    update_signal(&kb.key_0, window.key_0); update_signal(&kb.key_1, window.key_1);
    update_signal(&kb.key_2, window.key_2); update_signal(&kb.key_3, window.key_3);
    update_signal(&kb.key_4, window.key_4); update_signal(&kb.key_5, window.key_5);
    update_signal(&kb.key_6, window.key_6); update_signal(&kb.key_7, window.key_7);
    update_signal(&kb.key_8, window.key_8); update_signal(&kb.key_9, window.key_9);
    update_signal(&kb.key_arrow_up,    window.key_arrow_up);
    update_signal(&kb.key_arrow_down,  window.key_arrow_down);
    update_signal(&kb.key_arrow_left,  window.key_arrow_left);
    update_signal(&kb.key_arrow_right, window.key_arrow_right);
    update_signal(&kb.key_space,     window.key_space);
    update_signal(&kb.key_enter,     window.key_enter);
    update_signal(&kb.key_escape,    window.key_escape);
    update_signal(&kb.key_tab,       window.key_tab);
    update_signal(&kb.key_backspace, window.key_backspace);
    update_signal(&kb.key_capslock,  window.key_capslock);
    update_signal(&kb.key_shift_l,   window.key_shift_l);
    update_signal(&kb.key_shift_r,   window.key_shift_r);
    update_signal(&kb.key_ctrl_l,    window.key_ctrl_l);
    update_signal(&kb.key_ctrl_r,    window.key_ctrl_r);
    update_signal(&kb.key_alt_left,  window.key_alt_l);
    update_signal(&kb.key_alt_right, window.key_alt_r);
	update_signal(&kb.key_period, window.key_period);

    // -----------------------------------------------
    // BUFFER INPUTS INTO player
    // -----------------------------------------------
    player.input_move_left  = kb.key_a.hold;
    player.input_move_right = kb.key_d.hold;
    player.input_move_up    = kb.key_w.hold;
    player.input_move_down  = kb.key_s.hold;
    player.input_jump_hold  = kb.key_space.hold;
    player.input_wall_press = kb.key_a.hold || kb.key_d.hold;
    if(kb.key_space.click)                           player.input_jump_click  = 1;
    if(kb.key_shift_l.click || kb.key_shift_r.click) player.input_dash        = 1;
    if(kb.key_f.click)                               player.input_shoot       = 1;
    if(kb.key_e.click)                               player.input_interact    = 1;
    if(kb.key_k.click)                               player.input_reload_world= 1;
    if(kb.key_g.click){
        player.god_mode = !player.god_mode;          // local mirror always (camera/render)
        player.input_god_mode_toggle = 1;            // host applies this to p2 via net
    }

    // -----------------------------------------------
    // LAN LOBBY
    // -----------------------------------------------
    if(game_state == STATE_NET_LOBBY){
        // feed printable chars into the active field
        // key_signal is a keycode, not ASCII — map digit keys manually
        { char c = 0;
          if(kb.key_0.click) c='0'; else if(kb.key_1.click) c='1';
          else if(kb.key_2.click) c='2'; else if(kb.key_3.click) c='3';
          else if(kb.key_4.click) c='4'; else if(kb.key_5.click) c='5';
          else if(kb.key_6.click) c='6'; else if(kb.key_7.click) c='7';
          else if(kb.key_8.click) c='8'; else if(kb.key_9.click) c='9';
          if(c) lan_lobby_type_char(&lan, c);
          if(lan.ui_focus == 1 && kb.key_period.click) lan_lobby_type_char(&lan, '.');
        }
        if(kb.key_backspace.click)           lan_lobby_backspace(&lan);
        if(kb.key_tab.click)                 lan_lobby_tab(&lan);

        if(kb.key_enter.click){
            if(!lan.connected){
                // open socket with whatever is typed
                if(lan.role == LAN_HOST) lan_start_host(&lan);
                else                     lan_start_client(&lan);
            } else if(lan.role == LAN_HOST){
                // client already joined — begin game
                lan_send_start(&lan);
                stop_sound(&sfx.bgm_menu);
                play_sound_loop(&sfx.bgm_play_low_layer);
                game_state = STATE_PLAY;
                reset_game();
            }
            // client: game_state flips via snapshot from host
        }

        /* click on a discovered host entry to select it */
        if(mouse.left.click && lan.role == LAN_CLIENT && !lan.sock_open){
            int mx2=mouse.x, my2=mouse.y;
            for(int _i=0; _i<lan.discovered_count && _i<8; _i++){
                if(gui_hit(gui.disc_entries[_i], mx2, my2)){
                    lan_select_discovered(&lan, _i);
                    break;
                }
            }
        }

        if(kb.key_escape.click){ lan_stop(&lan); lan_disc_close(&lan); game_state = STATE_MENU; }
        return 1;
    }

    // -----------------------------------------------
    // MENU
    // -----------------------------------------------
    if(game_state == STATE_MENU){
        if(menu_sub_state == 0){
            if(kb.key_enter.click){
                play_sound_loop(&sfx.bgm_play_low_layer);
                stop_sound(&sfx.bgm_menu);
                game_state = STATE_PLAY;
                reset_game();
            }
            if(mouse.left.click){
                int mx=mouse.x, my=mouse.y;
                if(gui_hit(gui.menu_new_game,mx,my)){
                    play_sound_loop(&sfx.bgm_play_low_layer);
                    stop_sound(&sfx.bgm_menu);
                    game_state=STATE_PLAY; reset_game();
                }
                if(gui_hit(gui.menu_options, mx,my)){ menu_sub_state=1; }
                if(gui_hit(gui.menu_about,   mx,my)){ menu_sub_state=2; }
                if(gui_hit(gui.menu_host,    mx,my)){
                    lan_init_ctx(&lan);
                    lan.role = LAN_HOST;
                    game_state = STATE_NET_LOBBY;
                }
                if(gui_hit(gui.menu_join,    mx,my)){
                    lan_init_ctx(&lan);
                    lan.role = LAN_CLIENT;
                    lan_disc_open(&lan);   /* start listening for host broadcasts */
                    game_state = STATE_NET_LOBBY;
                }
                if(gui_hit(gui.menu_exit,    mx,my)){ return 0; }
            }
        } else {
            if(kb.key_escape.click) menu_sub_state=0;
        }
        return 1;
    }

    // PAUSE TOGGLE
    if(game_state==STATE_PLAY || game_state==STATE_PAUSE){
        if(kb.key_escape.click)
            game_state = (game_state==STATE_PLAY) ? STATE_PAUSE : STATE_PLAY;
    }

    // PAUSE MENU BUTTON
    if(game_state==STATE_PAUSE && mouse.left.click){
        if(gui_hit(gui.pause_main_menu, mouse.x, mouse.y)){
            game_state=STATE_MENU; menu_sub_state=0;
        }
    }

    // WIN SCREEN
    if(game_state == STATE_WIN){
        if(kb.key_space.click){ game_state=STATE_PLAY; reset_game(); }
        if(mouse.left.click){
            if(gui_hit(gui.win_play_again, mouse.x, mouse.y)){ game_state=STATE_PLAY; reset_game(); }
            if(gui_hit(gui.win_main_menu,  mouse.x, mouse.y)){ game_state=STATE_MENU; menu_sub_state=0; }
        }
        return 1;
    }

    // I/O = zoom in/out
    if(kb.key_i.click || kb.key_o.click){
        if(kb.key_i.click){ cfg.px_per_m += 10.0; if(cfg.px_per_m > 300.0) cfg.px_per_m = 300.0; }
        if(kb.key_o.click){ cfg.px_per_m -= 10.0; if(cfg.px_per_m <  20.0) cfg.px_per_m =  20.0; }
        double pm=cfg.px_per_m, sw=cfg.screen_w, sh=cfg.screen_h;
        camera.x_px = player.base.x*pm - sw/2.0;
        camera.y_px = player.base.y*pm - sh/2.0;
    }

    return 1;
}

#endif