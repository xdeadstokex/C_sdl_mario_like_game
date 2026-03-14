#ifndef IO_LOGIC_H
#define IO_LOGIC_H




//############  mouse section  ##################
typedef struct {
int hold;
int hold_last;
int click;
int unclick;
} signal_data;


struct mouse_data{
int x; int y;
int x_last; int y_last;
int dx; int dy;
int vx; int vy;
int vx_last; int vy_last;
int motion;

int click;

signal_data left;
signal_data middle;
signal_data right;
/*
int hold_left; int hold_left_last;
int click_left; int unclick_left;

int hold_right; int hold_right_last;
int click_right; int unclick_right;

int hold_middle; int hold_middle_last;
int click_middle; int unclick_middle;
*/
int scroll;
int scroll_v;

void* grab;
void (*gui_process)(void);
int new_grab; // to check whether new grab
};


struct kb_data{
int key_signal;
signal_data key_a; signal_data key_b; signal_data key_c;
signal_data key_d; signal_data key_e; signal_data key_f;
signal_data key_g; signal_data key_h; signal_data key_i;
signal_data key_j; signal_data key_k; signal_data key_l;
signal_data key_m; signal_data key_n; signal_data key_o;
signal_data key_p; signal_data key_q; signal_data key_r;
signal_data key_s; signal_data key_t; signal_data key_u;
signal_data key_v; signal_data key_w; signal_data key_x;
signal_data key_y; signal_data key_z;

signal_data key_0; signal_data key_1; signal_data key_2;
signal_data key_3; signal_data key_4; signal_data key_5;
signal_data key_6; signal_data key_7; signal_data key_8;
signal_data key_9;

signal_data key_arrow_up; signal_data key_arrow_down;
signal_data key_arrow_left; signal_data key_arrow_right;

signal_data key_space;
signal_data key_enter;
signal_data key_escape;
signal_data key_tab;
signal_data key_backspace;
signal_data key_capslock;
signal_data key_shift_l; signal_data key_shift_r;
signal_data key_ctrl_l; signal_data key_ctrl_r;
signal_data key_alt_left; signal_data key_alt_right;
};


void set_mouse(struct mouse_data* mouse, int x, int y){
mouse->x = x; mouse->y = y;
mouse->x_last = x; mouse->y_last = y;
mouse->dx = 0; mouse->dy = 0;
mouse->vx = 0; mouse->vy = 0;
mouse->vx_last = 0; mouse->vy_last = 0;
mouse->click = 0;

mouse->left.hold = 0; mouse->left.hold_last = 0;
mouse->left.click = 0; mouse->left.unclick = 0;

mouse->middle.hold = 0; mouse->middle.hold_last = 0;
mouse->middle.click = 0; mouse->middle.unclick = 0;

mouse->right.hold = 0; mouse->right.hold_last = 0;
mouse->right.click = 0; mouse->right.unclick = 0;
/*
mouse->hold_left = 0; mouse->hold_left_last = 0;
mouse->click_left = 0; mouse->unclick_left = 0;

mouse->hold_right = 0; mouse->hold_right_last = 0;
mouse->click_right = 0; mouse->unclick_right = 0;

mouse->hold_middle = 0; mouse->hold_middle_last = 0;
mouse->click_middle = 0; mouse->unclick_middle = 0;
*/

mouse->scroll = 0;
mouse->scroll_v = 0;

mouse->grab = NULL;
mouse->gui_process = NULL;
return;
}


void move_mouse(struct mouse_data* mouse, double x, double y){
mouse->x_last = mouse->x;
mouse->y_last = mouse->y;
mouse->x = x;
mouse->y = y;
mouse->dx = x - mouse->x_last;
mouse->dy = y - mouse->y_last;
return;
}


void update_signal(void* input, int signal){
signal_data* signal_info = (signal_data*) input;
signal_info->hold_last = signal_info->hold;
signal_info->hold = signal;

signal_info->click = signal && !signal_info->hold_last;
signal_info->unclick = !signal && signal_info->hold_last;
return;
}




#endif
