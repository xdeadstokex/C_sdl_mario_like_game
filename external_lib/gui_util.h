#ifndef GUI_UTIL_H
#define GUI_UTIL_H

#include "common_logic.h"
#include "sdl2_wrapper.h"
//###############################################
// GUI  — button rects computed by render,
//         read by control for hit-testing.
// All values are screen pixels (top-left origin).
// x,y = top-left corner.  w,h = size.
//###############################################
typedef struct {
    int x,y,w,h;
    unsigned int box_col;
    unsigned int text_col;

    int text_sx, text_sy;   // scale
    int text_align;         // 0 center, 1 top-left

    char text[256];
} gui_rect_t;

// returns 1 if point (px,py) is inside rect r
static inline int gui_hit(gui_rect_t r, int px, int py){
    return check_point_in_box_2d(px, py, r.x, r.y, r.w, r.h);
}

typedef struct {
    // main menu
    gui_rect_t menu_new_game;
    gui_rect_t menu_options;
    gui_rect_t menu_about;
    gui_rect_t menu_host;
    gui_rect_t menu_join;
    gui_rect_t menu_exit;

    // pause screen
    gui_rect_t pause_main_menu;
    gui_rect_t pause_title;
    gui_rect_t pause_hint;

    // win screen
    gui_rect_t win_play_again;
    gui_rect_t win_main_menu;
    gui_rect_t win_title;
    gui_rect_t win_score;
    gui_rect_t win_hint;
	
    gui_rect_t menu_title_main;
    gui_rect_t menu_title_sub;
    gui_rect_t menu_title_hint;
    gui_rect_t menu_enter_hint;

    gui_rect_t menu_controls_title;
    gui_rect_t menu_ctrl[8];

    gui_rect_t menu_about_title;
    gui_rect_t menu_about_lines[7];
	
	// lan — discovered hosts list (client only)
    gui_rect_t disc_title;
    gui_rect_t disc_entries[8];   /* one per LAN_MAX_DISCOVERED */
    int        disc_entry_count;

	// lan
    gui_rect_t title;

    // PORT
    gui_rect_t port_label;
    gui_rect_t port_box;

    // IP
    gui_rect_t ip_label;
    gui_rect_t ip_box;

    // hints
    gui_rect_t hint_tab;
    gui_rect_t hint_enter;
    gui_rect_t hint_status;
    gui_rect_t hint_esc;
} gui_2d_data;


static inline gui_rect_t make_rect(
int x,int y,int w,int h,
unsigned int box_col,unsigned int text_col,
int sx,int sy,int align,
const char* t){
    gui_rect_t r;
    r.x=x; r.y=y; r.w=w; r.h=h;
    r.box_col=box_col;
    r.text_col=text_col;
    r.text_sx=sx;
    r.text_sy=sy;
    r.text_align=align;

    int i=0;
    for(; t[i] && i<255; i++) r.text[i]=t[i];
    r.text[i]=0;

    return r;
}

static void draw_btn(gui_rect_t r){
    int hov = gui_hit(r, mouse.x, mouse.y);

    unsigned int a = r.box_col & 0xFF;

    if(a != 0){
        unsigned int col = r.box_col;

        if(hov){
            unsigned int rgb = (col >> 8) + 0x40;
            if(rgb > 0xFFFFFF) rgb = 0xFFFFFF;
            col = (rgb << 8) | a;
        }

        draw_rect(&window,r.x,r.y,r.w,r.h,col);
        draw_rect(&window,r.x,r.y,r.w,2,0xFFFFFF55);
        draw_rect(&window,r.x,r.y+r.h-2,r.w,2,0x00000055);
    }

    if(r.text_align == 0){
        draw_text_centered(&window,&font,r.text,
            r.x + r.w/2,
            r.y + r.h/2 - 6,
            r.text_sx,r.text_sy,2,2,
            r.text_col);
    } else {
        draw_text(&window,&font,r.text,
            r.x,
            r.y,
            r.text_sx,r.text_sy,2,2,
            r.text_col);
    }
}

static inline void gui_set_text(gui_rect_t* r, const char* t){
    int i=0; for(; t[i] && i<255; i++) r->text[i]=t[i];
    r->text[i]=0;
}

static inline void gui_field(gui_rect_t* box, const char* val, int active, int blink){
    char buf[40];
    snprintf(buf,40,(active && blink)?"%s_":"%s",val);
    gui_set_text(box, buf);

    box->box_col = active ? 0x223344FF : 0x0D1A2AFF;
    draw_btn(*box);
}


void init_lobby_rects(gui_2d_data* gui, int sw, int sh){
    int cx = sw/2;
    int fw = sw * 28 / 100;
    int fh = sh * 6 / 100;
    int fx = cx - fw/2;

    gui->title = make_rect(cx-fw/2, sh*20/100, fw, fh, 0x00000000, 0x88BBDDFF, 4,4,0, "");
    gui->port_label = make_rect(fx, sh*35/100, fw, fh, 0x00000000, 0x7090B0FF, 1,1,1, "PORT:");
    gui->port_box = make_rect(fx, sh*38/100, fw, fh, 0x0D1A2AFF, 0xFFFFFFFF, 2,2,0, "");
    gui->ip_label = make_rect(fx, sh*51/100, fw, fh, 0x00000000, 0x7090B0FF, 1,1,1, "HOST IP:");
    gui->ip_box = make_rect(fx, sh*54/100, fw, fh, 0x0D1A2AFF, 0xFFFFFFFF, 2,2,0, "");
    gui->hint_tab = make_rect(fx, sh*66/100, fw, fh, 0x00000000, 0x445566FF, 1,1,0, "TAB  SWITCH FIELD");
    gui->hint_enter = make_rect(fx, sh*78/100, fw, fh, 0x00000000, 0x44FF88FF, 1,1,0, "");
    gui->hint_status = make_rect(fx, sh*74/100, fw, fh, 0x00000000, 0xFFAA44FF, 1,1,0, "");
    gui->hint_esc = make_rect(fx, sh*90/100, fw, fh, 0x00000000, 0x445566FF, 1,1,0, "ESC  CANCEL");

    /* discovered hosts list — shown on right side for CLIENT */
    int dx = cx + fw/2 + sw*3/100;   /* right of the manual fields */
    int dw = fw;
    int dh = fh;
    gui->disc_title = make_rect(dx, sh*35/100, dw, dh, 0x00000000, 0x7090B0FF, 1,1,0, "VISIBLE HOSTS");
    for(int i = 0; i < 8; i++){
        gui->disc_entries[i] = make_rect(dx, sh*38/100 + i*(dh+4), dw, dh,
            0x0D1A2AFF, 0xFFFFFFFF, 1,1,0, "");
    }
    gui->disc_entry_count = 0;
}

void init_menu_rects(gui_2d_data* gui, int sw, int sh){
    int cx = sw/2;
    int bw = sw/6;
    int bh = sh*6/100;
    int bx = cx - bw/2;

    // menu
    gui->menu_new_game = make_rect(bx,sh*34/100,bw,bh,0x1A4A2AFF,0xFFFFFFFF, 2,2,0,"NEW GAME");
    gui->menu_options  = make_rect(bx,sh*42/100,bw,bh,0x1A2A4AFF,0xFFFFFFFF, 2,2,0,"OPTIONS");
    gui->menu_about    = make_rect(bx,sh*50/100,bw,bh,0x2A1A4AFF,0xFFFFFFFF, 2,2,0,"ABOUT");
    gui->menu_host     = make_rect(bx,sh*58/100,bw,bh,0x1A3A4AFF,0xFFFFFFFF, 2,2,0,"HOST");
    gui->menu_join     = make_rect(bx,sh*66/100,bw,bh,0x2A3A1AFF,0xFFFFFFFF, 2,2,0,"JOIN");
    gui->menu_exit     = make_rect(bx,sh*74/100,bw,bh,0x4A1A1AFF,0xFFFFFFFF, 2,2,0,"EXIT");

    // win buttons
    gui->win_play_again = make_rect(bx,sh*50/100,bw,bh,0x1A4A2AFF,0xFFFFFFFF, 2,2,0, "PLAY AGAIN");
    gui->win_main_menu  = make_rect(bx,sh*58/100,bw,bh,0x2A2A4AFF,0xFFFFFFFF, 2,2,0, "MAIN MENU");

    // win texts (no box alpha 0)
    gui->win_title = make_rect(cx, gui->win_play_again.y - bh*2, 0,0,0x00000000,0xFFFF88FF, 4,4,0, "SUMMIT REACHED");
    gui->win_score = make_rect(cx, gui->win_play_again.y - bh,   0,0,0x00000000,0xFFD700FF, 4,4,0, "");
    gui->win_hint  = make_rect(cx, gui->win_main_menu.y + bh + bh/2, 0,0,0x00000000,0x667788FF, 4,4,0, "SPACE TO REPLAY");

    // pause button 
	gui->pause_main_menu = make_rect(bx,sh*56/100,bw,bh,0x2A2A4AFF,0xFFFFFFFF, 2,2,0, "MAIN MENU");

    // pause texts
    gui->pause_title = make_rect(cx, gui->pause_main_menu.y - bh,   0,0,0x00000000,0xFFFFFFFF, 4,4,0,"PAUSED");
    gui->pause_hint  = make_rect(cx, gui->pause_main_menu.y - bh/2, 0,0,0x00000000,0x8899AAFF, 2,2,0, "ESC TO RESUME");

    // ===== MENU TEXT =====

    // titles
    gui->menu_title_main   = make_rect(cx, sh*12/100, 0,0, 0x00000000,0xC8DCF0FF, 6,6,0, "VERTICAL CLIMBER");
    gui->menu_title_hint   = make_rect(cx, sh*19/100, 0,0, 0x00000000,0x7090B0FF, 2,2,0, "REACH THE SUMMIT");
    // enter hint (below last button)
    gui->menu_enter_hint   = make_rect(cx, sh*25/100, 0,0, 0x00000000,0x445566FF, 4,4,0, "ENTER FOR NEW GAME");

    // controls title
    gui->menu_controls_title = make_rect(cx, sh*33/100, 0,0, 0x00000000,0xC8DCF0FF, 4,4,0, "CONTROLS");

    // controls lines
    int lx = cx - sw/6;
    int ly = sh*39/100;
    int ls = sh*23/1000;

    gui->menu_ctrl[0] = make_rect(lx,ly+ls*0,0,0,0x00000000,0xFFFFFFFF, 1,1,1, "A D      MOVE");
    gui->menu_ctrl[1] = make_rect(lx,ly+ls*1,0,0,0x00000000,0xFFFFFFFF, 1,1,1, "SPACE    JUMP");
    gui->menu_ctrl[2] = make_rect(lx,ly+ls*2,0,0,0x00000000,0xFFFFFFFF, 1,1,1, "SHIFT    DASH");
    gui->menu_ctrl[3] = make_rect(lx,ly+ls*3,0,0,0x00000000,0xFFFFFFFF, 1,1,1, "A OR D   WALL GRAB");
    gui->menu_ctrl[4] = make_rect(lx,ly+ls*4,0,0,0x00000000,0xFFFFFFFF, 1,1,1, "G        GOD MODE");
    gui->menu_ctrl[5] = make_rect(lx,ly+ls*5,0,0,0x00000000,0xFFFFFFFF, 1,1,1, "K        RELOAD WORLD");
    gui->menu_ctrl[6] = make_rect(lx,ly+ls*6,0,0,0x00000000,0xFFFFFFFF, 1,1,1, "ESC      PAUSE");
    gui->menu_ctrl[7] = make_rect(lx,ly+ls*8,0,0,0x00000000,0x8899AAFF, 1,1,1, "ESC      BACK");

    // about title
    gui->menu_about_title = make_rect(cx, sh*33/100, 0,0, 0x00000000,0xC8DCF0FF, 4,4,0, "ABOUT");

    // about lines
    ly = sh*39/100;
    ls = sh*25/1000;

    gui->menu_about_lines[0] = make_rect(lx,ly+ls*0,0,0,0x00000000,0xFFFFFFFF, 1,1,1, "MADE WITH SDL2 AND C");
    gui->menu_about_lines[1] = make_rect(lx,ly+ls*1,0,0,0x00000000,0xFFFFFFFF, 1,1,1, "CLIMB 100M TO WIN");
    gui->menu_about_lines[2] = make_rect(lx,ly+ls*2,0,0,0x00000000,0xFFFFFFFF, 1,1,1, "STOMP ENEMIES");
    gui->menu_about_lines[3] = make_rect(lx,ly+ls*3,0,0,0x00000000,0xFFFFFFFF, 1,1,1, "BREAK CRACKED ROCKS");
    gui->menu_about_lines[4] = make_rect(lx,ly+ls*4,0,0,0x00000000,0xFFFFFFFF, 1,1,1, "COLLECT GOLD COINS");
    gui->menu_about_lines[5] = make_rect(lx,ly+ls*5,0,0,0x00000000,0xFFFFFFFF, 1,1,1, "GET STAR FOR BOOST");
    gui->menu_about_lines[6] = make_rect(lx,ly+ls*7,0,0,0x00000000,0x8899AAFF, 1,1,1, "ESC  BACK");

	init_lobby_rects(gui, sw, sh);
}

#endif