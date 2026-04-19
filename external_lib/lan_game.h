#ifndef LAN_GAME_H
#define LAN_GAME_H

/*
  lan_game.h — 2-player LAN (host + 1 client), non-blocking UDP

  FLOW:
    HOST  : press Enter -> bind socket -> wait for MSG_JOIN
            on join -> show "P2 joined", wait for host Enter
            host presses Enter -> lan_send_start -> both enter STATE_PLAY
            ALSO: every tick in lobby, broadcast MSG_ANNOUNCE on DISCOVERY_PORT
                  so clients can see the host without typing IP manually.

    CLIENT: press Enter -> bind socket -> send MSG_JOIN every tick
            on MSG_START from host -> set connected=1, game_state=STATE_PLAY
            then send input each tick, receive state snapshots
            ALSO: before pressing Enter, client listens on DISCOVERY_PORT
                  and fills a discovered-hosts list shown in the lobby UI.
                  Clicking/selecting a discovered host fills ui_ip + ui_port.

  PORT SCHEME:
    host binds  host_port      (e.g. 7777)
    client binds host_port+1   (e.g. 7778)
    host sends state  -> peer_ip : host_port+1
    client sends join/input -> peer_ip : host_port
    discovery: host broadcasts -> 255.255.255.255 : DISCOVERY_PORT (7776)
               client listens on DISCOVERY_PORT (separate socket, always open)
*/

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "network_util.h"
#include "game_obj.h"

//###############################################
// CONSTANTS
//###############################################
#define LAN_DEFAULT_HOST_PORT  7777
#define LAN_DEFAULT_IP         "127.0.0.1"
#define LAN_DISCOVERY_PORT     7776   /* broadcast channel, separate from game port */
#define LAN_ANNOUNCE_INTERVAL  30     /* ticks between announce broadcasts */
#define LAN_HOST_TIMEOUT       180    /* ticks before a discovered host expires */
#define LAN_MAX_DISCOVERED     8      /* max hosts shown in client list */

#define LAN_OFF    0
#define LAN_HOST   1
#define LAN_CLIENT 2

#define MSG_INPUT    0x01
#define MSG_STATE    0x02
#define MSG_JOIN     0x03
#define MSG_START    0x04
#define MSG_ANNOUNCE 0x05   /* host -> broadcast: "I am hosting on port X" */
#define MSG_ACK      0x06
//###############################################
// WIRE STRUCTS
//###############################################
#pragma pack(push,1)

typedef struct {
    unsigned char msg;
    unsigned char move_left, move_right, move_up, move_down;
    unsigned char jump_click, jump_hold;
    unsigned char dash, shoot, interact, reload_world, wall_press;
    unsigned char god_mode_toggle;
    int           last_move_dir;
} net_input_t;

typedef struct {
    float x, y, vx, vy;
    int   hp, score, invincible, dashing, dash_dir;
    int   edge_grab, grab_wall_dir, last_dir, on_ground;
    int   jump_boost, speed_boost, slow_timer, fireball_ammo;
} net_player_t;

typedef struct {
    unsigned char  msg;
    int            game_state, tick;
    net_player_t   p1, p2;
    int            enemy_count;
    struct { float x,y,vx,vy; int type,hp,active,stun_timer,dashing; } enemies[ENEMY_COUNT];
    int            coin_count;
    struct { float x,y; int collected; }               coins[COIN_COUNT];
    struct { float x,y; int active,dir,type; }         projs[PROJ_COUNT];
    int            terrain_count;
    struct { int broken,broken_timer,warning_timer,hp; } terrains[TERRAIN_COUNT];
    int            chest_count;
    struct { int state; }                              chests[CHEST_COUNT];
    int            item_count;
    struct { float x,y; int active,type; }             items[ITEM_COUNT];
    int            pobj_count;
    struct { float x,y,vx,vy; int active; unsigned int color; } pobjs[POBJ_COUNT];
} net_state_t;

typedef struct {
    unsigned char  msg;        /* MSG_ANNOUNCE */
    unsigned short host_port;
    char           name[16];   /* optional display name */
} net_announce_t;

#pragma pack(pop)

//###############################################
// DISCOVERED HOST ENTRY
//###############################################
typedef struct {
    char           ip[32];
    unsigned short port;
    char           name[16];
    int            ttl;   /* countdown in ticks; 0 = expired */
} lan_host_entry_t;

//###############################################
// CONTEXT
//###############################################
typedef struct {
    int            role;       /* LAN_OFF / LAN_HOST / LAN_CLIENT */
    int            sock_open;
    network_t      sock;
    char           peer_ip[32];
    int            connected;
	int            client_confirmed;  // 1 = received at least one MSG_INPUT
    int            tick;
    unsigned short host_port;
    char           ui_ip[32];
    char           ui_port[8];
    int            ui_focus;   /* 0=port, 1=ip */
    struct player_data p2;
    net_input_t    last_input;
    char           status_msg[64];

    /* discovery */
    int              disc_open;
    network_t        disc_sock;
    int              announce_timer;
    lan_host_entry_t discovered[LAN_MAX_DISCOVERED];
    int              discovered_count;
    int              selected_host;   /* -1 = none */
} lan_ctx_t;

//###############################################
// INIT / STOP
//###############################################
static inline void lan_init_ctx(lan_ctx_t* lan){
    /* preserve discovery state so client keeps seeing hosts across re-inits */
    int disc_open = lan->disc_open;
    network_t disc_sock = lan->disc_sock;
    int disc_count = lan->discovered_count;
    lan_host_entry_t disc_copy[LAN_MAX_DISCOVERED];
    memcpy(disc_copy, lan->discovered, sizeof(disc_copy));

    memset(lan, 0, sizeof(lan_ctx_t));
    lan->host_port = LAN_DEFAULT_HOST_PORT;
    strcpy(lan->peer_ip, LAN_DEFAULT_IP);
    strcpy(lan->ui_ip,   LAN_DEFAULT_IP);
    snprintf(lan->ui_port, 8, "%d", LAN_DEFAULT_HOST_PORT);
    strcpy(lan->status_msg, "Offline");
    lan->selected_host = -1;

    lan->disc_open = disc_open;
    lan->disc_sock = disc_sock;
    lan->discovered_count = disc_count;
    memcpy(lan->discovered, disc_copy, sizeof(disc_copy));
}

static inline void lan_stop(lan_ctx_t* lan){
    if(lan->sock_open){ net_close(&lan->sock); lan->sock_open = 0; }
    char saved_ip[32], saved_port[8];
    strcpy(saved_ip,   lan->ui_ip);
    strcpy(saved_port, lan->ui_port);
    lan_init_ctx(lan);
    strcpy(lan->ui_ip,   saved_ip);
    strcpy(lan->ui_port, saved_port);
	lan->connected = 0;
	lan->client_confirmed = 0;
}

/* Open client discovery listen socket. Call once on entering JOIN lobby. */
static inline void lan_disc_open(lan_ctx_t* lan){
    if(lan->disc_open) return;
    net_init(&lan->disc_sock, LAN_DISCOVERY_PORT, 0 /* non-blocking */);
    lan->disc_open = 1;
}

static inline void lan_disc_close(lan_ctx_t* lan){
    if(!lan->disc_open) return;
    net_close(&lan->disc_sock);
    lan->disc_open = 0;
    lan->discovered_count = 0;
    lan->selected_host = -1;
}

static inline void lan_start_host(lan_ctx_t* lan){
    if(lan->sock_open) return;
    int p = atoi(lan->ui_port);
    lan->host_port = (p > 0 && p < 65536) ? (unsigned short)p : LAN_DEFAULT_HOST_PORT;
    net_init(&lan->sock, lan->host_port, 0);
    lan->sock_open = 1;
    lan->announce_timer = 0;
    snprintf(lan->status_msg, 64, "Hosting :%d — waiting for client...", lan->host_port);
    printf("[LAN] Host on port %d\n", lan->host_port);
}

static inline void lan_start_client(lan_ctx_t* lan){
    if(lan->sock_open) return;
    int p = atoi(lan->ui_port);
    lan->host_port = (p > 0 && p < 65536) ? (unsigned short)p : LAN_DEFAULT_HOST_PORT;
    strncpy(lan->peer_ip, lan->ui_ip[0] ? lan->ui_ip : LAN_DEFAULT_IP, 31);
    lan->peer_ip[31] = '\0';
    net_init(&lan->sock, lan->host_port + 1, 0);
    lan->sock_open = 1;
    snprintf(lan->status_msg, 64, "Joining %s:%d ...", lan->peer_ip, lan->host_port);
    printf("[LAN] Client -> %s:%d\n", lan->peer_ip, lan->host_port);
}

//###############################################
// DISCOVERY
//###############################################

/* Host: broadcast presence every ANNOUNCE_INTERVAL ticks */
static inline void lan_broadcast_announce(lan_ctx_t* lan){
    if(!lan->sock_open) return;
    if(--lan->announce_timer > 0) return;
    lan->announce_timer = LAN_ANNOUNCE_INTERVAL;

    net_announce_t pkt;
    pkt.msg = MSG_ANNOUNCE;
    pkt.host_port = lan->host_port;
    strncpy(pkt.name, "HOST", 15); pkt.name[15] = '\0';

    char ip[32];

    for(int a = 0; a < 256; a++){
        for(int b = 1; b < 255; b++){
            snprintf(ip, sizeof(ip), "192.168.%d.%d", a, b);
            net_send(&lan->sock, ip, LAN_DISCOVERY_PORT, &pkt, sizeof(pkt));
        }
    }
}

/* Client: drain discovery socket, update discovered list */
static inline void lan_disc_poll(lan_ctx_t* lan){
    if(!lan->disc_open) return;

    /* age entries */
    for(int i = 0; i < lan->discovered_count; ){
        lan->discovered[i].ttl--;
        if(lan->discovered[i].ttl <= 0){
            int last = --lan->discovered_count;
            if(lan->selected_host == i)    lan->selected_host = -1;
            else if(lan->selected_host == last) lan->selected_host = i;
            lan->discovered[i] = lan->discovered[last];
        } else {
            i++;
        }
    }

    static unsigned char buf[64];
    char from_ip[32]; int n;
    while((n = net_recv(&lan->disc_sock, buf, sizeof(buf), from_ip)) > 0){
        if(n < (int)sizeof(net_announce_t)) continue;
        net_announce_t* pkt = (net_announce_t*)buf;
        if(pkt->msg != MSG_ANNOUNCE) continue;

        int found = 0;
        for(int i = 0; i < lan->discovered_count; i++){
            if(strcmp(lan->discovered[i].ip, from_ip)==0 &&
               lan->discovered[i].port == pkt->host_port){
                lan->discovered[i].ttl = LAN_HOST_TIMEOUT;
                found = 1; break;
            }
        }
        if(!found && lan->discovered_count < LAN_MAX_DISCOVERED){
            lan_host_entry_t* e = &lan->discovered[lan->discovered_count++];
            strncpy(e->ip, from_ip, 31); e->ip[31] = '\0';
            e->port = pkt->host_port;
            strncpy(e->name, pkt->name, 15); e->name[15] = '\0';
            e->ttl = LAN_HOST_TIMEOUT;
        }
    }
}

/* Client clicks/selects a discovered host — fills ui_ip/ui_port */
static inline void lan_select_discovered(lan_ctx_t* lan, int idx){
    if(idx < 0 || idx >= lan->discovered_count) return;
    lan->selected_host = idx;
    strncpy(lan->ui_ip,   lan->discovered[idx].ip, 31);
    lan->ui_ip[31] = '\0';
    snprintf(lan->ui_port, 8, "%d", lan->discovered[idx].port);
}

//###############################################
// SEND HELPERS
//###############################################
static inline void lan_send_join(lan_ctx_t* lan){
    unsigned char b[2] = {MSG_JOIN, 0};
    net_send(&lan->sock, lan->peer_ip, lan->host_port, b, 2);
}

static inline void lan_send_start(lan_ctx_t* lan){
    unsigned char b[2] = {MSG_START, 0};
    for(int i = 0; i < 10; i++){
        net_send(&lan->sock, lan->peer_ip, lan->host_port + 1, b, 2);
	}
}

static inline void lan_send_input(lan_ctx_t* lan, net_input_t* i){
    net_send(&lan->sock, lan->peer_ip, lan->host_port, i, sizeof(net_input_t));
}

static inline void lan_send_state(lan_ctx_t* lan, net_state_t* s){
    net_send(&lan->sock, lan->peer_ip, lan->host_port + 1, s, sizeof(net_state_t));
}

//###############################################
// PACK / UNPACK PLAYER
//###############################################
static inline net_player_t lan_pack_player(struct player_data* p){
    return (net_player_t){
        (float)p->base.x, (float)p->base.y, (float)p->base.vx, (float)p->base.vy,
        p->hp, p->score, p->invincible, p->dashing, p->dash_dir,
        p->edge_grab, p->grab_wall_dir, p->last_move_dir, p->on_ground,
        p->jump_boost_timer, p->speed_boost_timer, p->slow_timer, p->fireball_ammo
    };
}

static inline void lan_unpack_player(net_player_t* s, struct player_data* p){
    p->base.x = s->x; p->base.y = s->y;
    p->base.vx = s->vx; p->base.vy = s->vy;
    p->hp = s->hp; p->score = s->score;
    p->invincible = s->invincible;
    p->dashing = s->dashing; p->dash_dir = s->dash_dir;
    p->edge_grab = s->edge_grab; p->grab_wall_dir = s->grab_wall_dir;
    p->last_move_dir = s->last_dir;
    p->on_ground = s->on_ground;
    p->jump_boost_timer  = s->jump_boost;
    p->speed_boost_timer = s->speed_boost;
    p->slow_timer        = s->slow_timer;
    p->fireball_ammo     = s->fireball_ammo;
}

//###############################################
// PACK STATE  (host -> wire)
//###############################################
static inline void lan_pack_state(lan_ctx_t* lan, net_state_t* st){
    memset(st, 0, sizeof(net_state_t));
    st->msg = MSG_STATE; st->game_state = game_state; st->tick = lan->tick;
    st->p1 = lan_pack_player(&player);
    st->p2 = lan_pack_player(&lan->p2);

    st->enemy_count = enemy_count_actual;
    for(int i = 0; i < enemy_count_actual; i++){
        st->enemies[i].x  = enemies[i].base.x;  st->enemies[i].y  = enemies[i].base.y;
        st->enemies[i].vx = enemies[i].base.vx; st->enemies[i].vy = enemies[i].base.vy;
        st->enemies[i].type        = enemies[i].type;
        st->enemies[i].hp          = enemies[i].hp;
        st->enemies[i].active      = enemies[i].active;
        st->enemies[i].stun_timer  = enemies[i].stun_timer;
        st->enemies[i].dashing     = enemies[i].dashing;
    }
    st->coin_count = coin_count_actual;
    for(int i = 0; i < coin_count_actual; i++){
        st->coins[i].x = coins[i].x; st->coins[i].y = coins[i].y;
        st->coins[i].collected = coins[i].collected;
    }
    for(int i = 0; i < PROJ_COUNT; i++){
        st->projs[i].x      = projectiles[i].x;
        st->projs[i].y      = projectiles[i].y;
        st->projs[i].active = projectiles[i].active;
        st->projs[i].dir    = projectiles[i].dir;
        st->projs[i].type   = projectiles[i].type;
    }
    st->terrain_count = terrain_count_actual;
    for(int i = 0; i < terrain_count_actual; i++){
        st->terrains[i].broken       = terrains[i].broken;
        st->terrains[i].broken_timer = terrains[i].broken_timer;
        st->terrains[i].warning_timer= terrains[i].warning_timer;
        st->terrains[i].hp           = terrains[i].hp;
    }
    st->chest_count = chest_count_actual;
    for(int i = 0; i < chest_count_actual; i++){ st->chests[i].state = chests[i].state; }

    st->item_count = item_count_actual;
    for(int i = 0; i < item_count_actual; i++){
        st->items[i].x      = items[i].x;
        st->items[i].y      = items[i].y;
        st->items[i].active = items[i].active;
        st->items[i].type   = items[i].type;
    }
    st->pobj_count = pobj_count_actual;
    for(int i = 0; i < pobj_count_actual; i++){
        st->pobjs[i].x = (float)pobjs[i].base.x; st->pobjs[i].y = (float)pobjs[i].base.y;
        st->pobjs[i].vx= (float)pobjs[i].base.vx;st->pobjs[i].vy= (float)pobjs[i].base.vy;
        st->pobjs[i].active = pobjs[i].active;
        st->pobjs[i].color  = pobjs[i].color;
    }
}

//###############################################
// UNPACK STATE  (wire -> client globals)
//###############################################
static inline void lan_unpack_state(lan_ctx_t* lan, net_state_t* st){
    game_state = st->game_state;
    lan_unpack_player(&st->p2, &player);
    lan_unpack_player(&st->p1, &lan->p2);

    enemy_count_actual = st->enemy_count;
    for(int i = 0; i < enemy_count_actual; i++){
        enemies[i].base.x  = st->enemies[i].x;  enemies[i].base.y  = st->enemies[i].y;
        enemies[i].base.vx = st->enemies[i].vx; enemies[i].base.vy = st->enemies[i].vy;
        enemies[i].type       = st->enemies[i].type;
        enemies[i].hp         = st->enemies[i].hp;
        enemies[i].active     = st->enemies[i].active;
        enemies[i].stun_timer = st->enemies[i].stun_timer;
        enemies[i].dashing    = st->enemies[i].dashing;
    }
    coin_count_actual = st->coin_count;
    for(int i = 0; i < coin_count_actual; i++){
        coins[i].x = st->coins[i].x; coins[i].y = st->coins[i].y;
        coins[i].collected = st->coins[i].collected;
    }
    for(int i = 0; i < PROJ_COUNT; i++){
        projectiles[i].x      = st->projs[i].x;
        projectiles[i].y      = st->projs[i].y;
        projectiles[i].active = st->projs[i].active;
        projectiles[i].dir    = st->projs[i].dir;
        projectiles[i].type   = st->projs[i].type;
    }
    for(int i = 0; i < st->terrain_count && i < TERRAIN_COUNT; i++){
        terrains[i].broken        = st->terrains[i].broken;
        terrains[i].broken_timer  = st->terrains[i].broken_timer;
        terrains[i].warning_timer = st->terrains[i].warning_timer;
        terrains[i].hp            = st->terrains[i].hp;
    }
    for(int i = 0; i < st->chest_count && i < CHEST_COUNT; i++){ chests[i].state = st->chests[i].state; }

    item_count_actual = st->item_count;
    for(int i = 0; i < item_count_actual; i++){
        items[i].x      = st->items[i].x;
        items[i].y      = st->items[i].y;
        items[i].active = st->items[i].active;
        items[i].type   = st->items[i].type;
    }
    pobj_count_actual = st->pobj_count;
    for(int i = 0; i < pobj_count_actual; i++){
        pobjs[i].base.x  = st->pobjs[i].x; pobjs[i].base.y  = st->pobjs[i].y;
        pobjs[i].base.vx = st->pobjs[i].vx;pobjs[i].base.vy = st->pobjs[i].vy;
        pobjs[i].active  = st->pobjs[i].active;
        pobjs[i].color   = st->pobjs[i].color;
    }
}

//###############################################
// INPUT PACK / APPLY
//###############################################
static inline void lan_pack_input(net_input_t* inp){
    inp->msg          = MSG_INPUT;
    inp->move_left    = player.input_move_left;
    inp->move_right   = player.input_move_right;
    inp->move_up      = player.input_move_up;
    inp->move_down    = player.input_move_down;
    inp->jump_click   = player.input_jump_click;
    inp->jump_hold    = player.input_jump_hold;
    inp->dash         = player.input_dash;
    inp->shoot        = player.input_shoot;
    inp->interact     = player.input_interact;
    inp->reload_world = player.input_reload_world;
    inp->wall_press   = player.input_wall_press;
    inp->god_mode_toggle = player.input_god_mode_toggle;
    //inp->last_move_dir= player.last_move_dir; // dont resend this to host
    player.input_jump_click = player.input_dash    = player.input_shoot =
    player.input_interact   = player.input_reload_world =
    player.input_god_mode_toggle = 0;
}

static inline void lan_apply_input_to_p2(lan_ctx_t* lan){
    net_input_t* i = &lan->last_input;
    lan->p2.input_move_left  = i->move_left;
    lan->p2.input_move_right = i->move_right;
    lan->p2.input_move_up    = i->move_up;
    lan->p2.input_move_down  = i->move_down;
    lan->p2.input_jump_click|= i->jump_click;
    lan->p2.input_jump_hold  = i->jump_hold;
    lan->p2.input_dash      |= i->dash;
    lan->p2.input_shoot     |= i->shoot;
    lan->p2.input_interact  |= i->interact;
    lan->p2.input_wall_press = i->wall_press;
    if(i->god_mode_toggle)    lan->p2.god_mode ^= 1;
    if(i->last_move_dir)      lan->p2.last_move_dir = i->last_move_dir;
}

//###############################################
// HOST TICK
//###############################################
static inline void lan_host_tick(lan_ctx_t* lan){
    static unsigned char buf[sizeof(net_state_t) + 4];
    char from_ip[32]; int n;

    if(game_state == STATE_NET_LOBBY){ lan_broadcast_announce(lan); }

    while((n = net_recv(&lan->sock, buf, sizeof(buf), from_ip)) > 0){
        if(buf[0] == MSG_JOIN && !lan->connected){
            strncpy(lan->peer_ip, from_ip, 31);
            lan->peer_ip[31] = '\0';
            lan->connected = 1;
            snprintf(lan->status_msg, 64, "P2 joined: %s — press ENTER to start", from_ip);
            printf("[LAN] Client joined from %s\n", from_ip);
        }
        if(buf[0] == MSG_INPUT && lan->connected && n >= (int)sizeof(net_input_t)){
			lan->client_confirmed = 1;
            memcpy(&lan->last_input, buf, sizeof(net_input_t));
            lan_apply_input_to_p2(lan);
        }
    }

    if(lan->connected && game_state == STATE_PLAY || game_state == STATE_WIN){
		if(!lan->client_confirmed){
			lan_send_start(lan);  // keeps hammering until client responds
		}
        static net_state_t st;
        lan_pack_state(lan, &st);
        lan_send_state(lan, &st);
        lan->tick++;
		return;
    }

	if(buf[0] == MSG_ACK && lan->connected){
		lan->client_confirmed = 1;
	}
}

//###############################################
// CLIENT TICK
//###############################################
static inline void lan_client_tick(lan_ctx_t* lan){
    static unsigned char buf[sizeof(net_state_t) + 4];
    static net_state_t   st_buf;
    char from_ip[32]; int n;

    if(!lan->sock_open) return;

    if(!lan->connected){
        lan_send_join(lan);
    } else {
        static net_input_t inp;
        lan_pack_input(&inp);
        lan_send_input(lan, &inp);
    }

    net_state_t* latest = NULL;
    while((n = net_recv(&lan->sock, buf, sizeof(buf), from_ip)) > 0){
        if(buf[0] == MSG_START && !lan->connected){
            lan->connected = 1;
			// send ack confirm
			
			unsigned char ack[2] = {MSG_ACK, 0};
			for(int a = 0; a < 10; a += 1){
			net_send(&lan->sock, lan->peer_ip, lan->host_port, ack, 2);
			}
			
            strncpy(lan->peer_ip, from_ip, 31);
            lan->peer_ip[31] = '\0';
            snprintf(lan->status_msg, 64, "Connected to %s — starting!", lan->peer_ip);
            printf("[LAN] MSG_START received, entering game\n");
            game_state = STATE_PLAY;
        }
        if(buf[0] == MSG_STATE && lan->connected && n >= (int)sizeof(net_state_t)){
            memcpy(&st_buf, buf, sizeof(net_state_t));
            latest = &st_buf;
        }
    }
    if(latest) lan_unpack_state(lan, latest);
}

//###############################################
// LOBBY TEXT INPUT
//###############################################
static inline void lan_lobby_type_char(lan_ctx_t* lan, char c){
    char* f   = (lan->ui_focus == 1) ? lan->ui_ip   : lan->ui_port;
    int maxlen = (lan->ui_focus == 1) ? 31 : 7;
    int len    = (int)strlen(f);
    if(len < maxlen){ f[len] = c; f[len+1] = '\0'; }
}
static inline void lan_lobby_backspace(lan_ctx_t* lan){
    char* f = (lan->ui_focus == 1) ? lan->ui_ip : lan->ui_port;
    int len = (int)strlen(f); if(len > 0) f[len-1] = '\0';
}
static inline void lan_lobby_tab(lan_ctx_t* lan){
    if(lan->role == LAN_CLIENT) lan->ui_focus ^= 1;
}

//###############################################
// P2 RESET
//###############################################
static inline void lan_reset_p2(lan_ctx_t* lan){
    set_physic_base(&lan->p2.base,
        player.respawn_x + 1.0, player.respawn_y,
        0, 0, 0, cfg.gravity, 60, 0.4, 0.0,
        0, 0, cfg.player_w, cfg.player_h);
    lan->p2.on_ground = 0; lan->p2.on_wall = 0;
    lan->p2.edge_grab = 0; lan->p2.grab_wall_dir = 0;
    lan->p2.jump_count = 0; lan->p2.dash_ready = 1;
    lan->p2.dashing = 0; lan->p2.dash_dir = 1;
    lan->p2.invincible = 0; lan->p2.god_mode = 0;
    lan->p2.last_move_dir = 1;
    lan->p2.jump_boost_timer = lan->p2.speed_boost_timer = lan->p2.slow_timer = 0;
    lan->p2.hp = 5; lan->p2.fireball_ammo = 0; lan->p2.score = 0;
}

#endif /* LAN_GAME_H */
