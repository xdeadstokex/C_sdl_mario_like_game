#ifndef WORLD_LOADER_H
#define WORLD_LOADER_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "physic.h"
#include "game_obj.h"

//###############################################
// WORLD LOADER
// Depends on: physic.h, game_obj.h only.
//
// FILE FORMAT:
//   V  key  value     game config (meters, m/s, m/s²)
//   T  x  y  w  h  type(solid|break)  coin_inside(0|1)
//   E  x  y  type(dasher|boss)  patrol_min  patrol_max
//   C  x  y
//   I  x  y
//   P  x  y
//   O  x  y  w  h  friction  restitution  color(0xRRGGBBAA)
//   (all x,y = visual top-left in METERS)
//
// On parse error: fprintf(stderr) + exit(1)
//###############################################

typedef struct {
    const char*          filepath;
    int                  linenum;
    struct game_config*  cfg;
    struct terrain_data* terrains;
    int*                 terrain_count;
    int                  terrain_max;
    struct enemy_data*   enemies;
    int*                 enemy_count;
    int                  enemy_max;
    struct coin_data*    coins;
    int*                 coin_count;
    int                  coin_max;
    struct item_data*    items;
    int*                 item_count;
    int                  item_max;
    double*              spawn_x;
    double*              spawn_y;
    int                  spawn_set;
    struct chest_data* chests;
    int* chest_count;
    int chest_max;

    struct decor_data*   decors;
    int*                 decor_count;
    int                  decor_max;

    struct pobj_data*    pobjs;
    int*                 pobj_count;
    int                  pobj_max;

    // V key presence flags (1 = was set by file)
    int v_px_per_m, v_world_w, v_world_h;
    int v_gravity, v_move_speed, v_move_accel, v_move_friction;
    int v_jump_vy, v_jump_boost_vy, v_dash_speed, v_dash_frames;
    int v_edge_grab_slide, v_invincible_frames;
    int v_player_w, v_player_h, v_max_vx, v_max_vy;
    int p_present;  // at least one P tag seen

} wl_ctx;

//###############################################
// HELPERS
//###############################################
static void wl_error(wl_ctx* ctx, const char* msg, const char* line){
    fprintf(stderr,"[world_loader] ERROR %s line %d: %s\n  got: %s\n",
            ctx->filepath, ctx->linenum, msg, line);
    exit(1);
}
static void wl_check_fields(wl_ctx* ctx,int got,int need,const char* tag,const char* line){
    if(got!=need){
        char msg[128];
        sprintf(msg,"tag '%s' needs %d fields, got %d",tag,need,got);
        wl_error(ctx,msg,line);
    }
}
static void wl_check_cap(wl_ctx* ctx,int count,int max,const char* tag,const char* line){
    if(count>=max){
        char msg[128];
        sprintf(msg,"tag '%s' exceeded max %d",tag,max);
        wl_error(ctx,msg,line);
    }
}

//###############################################
// TAG HANDLERS
//###############################################

// V  key  value
static void load_tag_V(wl_ctx* ctx, const char* line){
    char key[64]; double val;
    int n=sscanf(line+1,"%63s %lf",key,&val);
    wl_check_fields(ctx,n,2,"V",line);
    struct game_config* c=ctx->cfg;
    if     (!strcmp(key,"px_per_m"))          { c->px_per_m          = val; ctx->v_px_per_m=1; }
    else if(!strcmp(key,"world_w"))           { c->world_w           = val; ctx->v_world_w=1; }
    else if(!strcmp(key,"world_h"))           { c->world_h           = val; ctx->v_world_h=1; }
    else if(!strcmp(key,"gravity"))           { c->gravity           = val; ctx->v_gravity=1; }
    else if(!strcmp(key,"move_speed"))        { c->move_speed        = val; ctx->v_move_speed=1; }
    else if(!strcmp(key,"move_accel"))        { c->move_accel        = val; ctx->v_move_accel=1; }
    else if(!strcmp(key,"move_friction"))     { c->move_friction     = val; ctx->v_move_friction=1; }
    else if(!strcmp(key,"jump_vy"))           { c->jump_vy           = val; ctx->v_jump_vy=1; }
    else if(!strcmp(key,"jump_boost_vy"))     { c->jump_boost_vy     = val; ctx->v_jump_boost_vy=1; }
    else if(!strcmp(key,"dash_speed"))        { c->dash_speed        = val; ctx->v_dash_speed=1; }
    else if(!strcmp(key,"dash_frames"))       { c->dash_frames       = (int)val; ctx->v_dash_frames=1; }
    else if(!strcmp(key,"edge_grab_slide"))   { c->edge_grab_slide   = val; ctx->v_edge_grab_slide=1; }
    else if(!strcmp(key,"invincible_frames")) { c->invincible_frames = (int)val; ctx->v_invincible_frames=1; }
    else if(!strcmp(key,"player_w"))          { c->player_w          = val; ctx->v_player_w=1; }
    else if(!strcmp(key,"player_h"))          { c->player_h          = val; ctx->v_player_h=1; }
    else if(!strcmp(key,"max_vx"))            { c->max_vx            = val; ctx->v_max_vx=1; }
    else if(!strcmp(key,"max_vy"))            { c->max_vy            = val; ctx->v_max_vy=1; }
    else { char msg[128]; sprintf(msg,"V: unknown key '%s'",key); wl_error(ctx,msg,line); }
}

// T  x  y  w  h  type  coin_inside  (top-left in meters)
static void load_tag_T(wl_ctx* ctx, const char* line){
    wl_check_cap(ctx,*ctx->terrain_count,ctx->terrain_max,"T",line);
    double x,y,w,h; char ts[32]; int ci;
    int n=sscanf(line+1,"%lf %lf %lf %lf %31s %d",&x,&y,&w,&h,ts,&ci);
    wl_check_fields(ctx,n,6,"T",line);
    int ttype,thp;
    if     (!strcmp(ts,"solid")){ ttype=TERRAIN_SOLID; thp=-1; }
    else if(!strcmp(ts,"break")){ ttype=TERRAIN_BREAK; thp= 1; }
    else wl_error(ctx,"T type must be solid|break",line);
    int i=*ctx->terrain_count;
    // file: y=0 ground, y increases upward
    // physic: y=0 top, y increases downward -> flip
    double py = ctx->cfg->world_h - y - h;  // physic top-left y
    set_physic_base(&ctx->terrains[i].base,
        x+w/2.0, py+h/2.0, 0,0,0,0,-1,0.4,0.0,0,0,w,h);
    ctx->terrains[i].type=ttype; ctx->terrains[i].hp=thp;
    ctx->terrains[i].broken=0;   ctx->terrains[i].broken_timer=0;
    ctx->terrains[i].coin_inside=ci;
    (*ctx->terrain_count)++;
}

// E  x  y  type  patrol_min  patrol_max  (top-left in meters)
static void load_tag_E(wl_ctx* ctx, const char* line){
    wl_check_cap(ctx,*ctx->enemy_count,ctx->enemy_max,"E",line);
    double x,y,pmin,pmax; char ts[32];
    int n=sscanf(line+1,"%lf %lf %31s %lf %lf",&x,&y,ts,&pmin,&pmax);
    wl_check_fields(ctx,n,5,"E",line);
    int etype; double cw,ch,dvx; int dt,hp;
    if(!strcmp(ts,"dasher")){
        etype=ENEMY_DASHER; cw=0.32; ch=0.32; dt=90; dvx=3.5; hp=1;
    } else if(!strcmp(ts,"boss")){
        etype=ENEMY_BOSS;   cw=0.64; ch=0.64; dt=45; dvx=5.2; hp=5;
    } else { wl_error(ctx,"E type must be dasher|boss",line); return; }
    int i=*ctx->enemy_count;
    double py = ctx->cfg->world_h - y - ch;  // physic top-left y
    set_physic_base(&ctx->enemies[i].base,
        x+cw/2.0, py+ch/2.0, 0,0,0,0,-1,0.4,0.0,0,0,cw,ch);
    ctx->enemies[i].type=etype;    ctx->enemies[i].active=1;
    ctx->enemies[i].stun_timer=0;  ctx->enemies[i].patrol_dir=1;
    ctx->enemies[i].patrol_timer=60;
    ctx->enemies[i].patrol_x_min=pmin; ctx->enemies[i].patrol_x_max=pmax;
    ctx->enemies[i].patrol_y=py+ch/2.0;  // physic center y
    ctx->enemies[i].dash_timer=dt; ctx->enemies[i].dashing=0;
    ctx->enemies[i].dash_vx=dvx;   ctx->enemies[i].hp=hp;
    (*ctx->enemy_count)++;
}

// C  x  y  (center in meters)
static void load_tag_C(wl_ctx* ctx, const char* line){
    wl_check_cap(ctx,*ctx->coin_count,ctx->coin_max,"C",line);
    double x,y; int n=sscanf(line+1,"%lf %lf",&x,&y);
    wl_check_fields(ctx,n,2,"C",line);
    int i=*ctx->coin_count;
    ctx->coins[i].x=x;
    ctx->coins[i].y=ctx->cfg->world_h - y;  // flip
    ctx->coins[i].collected=0;
    (*ctx->coin_count)++;
}

// I  x  y  type (center in meters)
static void load_tag_I(wl_ctx* ctx, const char* line){
    wl_check_cap(ctx,*ctx->item_count,ctx->item_max,"I",line);
    double x,y;
    int type = 2;
    int n = sscanf(line+1,"%lf %lf %d", &x, &y, &type);
    if(n < 2) wl_check_fields(ctx, n, 3, "I", line);
    
    int i=*ctx->item_count;
    ctx->items[i].x=x;
    ctx->items[i].y=ctx->cfg->world_h - y;  // flip
    ctx->items[i].active=1; 
    ctx->items[i].respawn_timer=0;
    ctx->items[i].type = type;
    (*ctx->item_count)++;
}

// B  x  y  item_type (top-left in meters)
static void load_tag_B(wl_ctx* ctx, const char* line){
    wl_check_cap(ctx,*ctx->chest_count,ctx->chest_max,"B",line);
    double x, y; int type;
    int n=sscanf(line+1,"%lf %lf %d",&x,&y,&type);
    wl_check_fields(ctx,n,3,"B",line);
    int i=*ctx->chest_count;
    ctx->chests[i].x = x + 0.3; // w/2
    ctx->chests[i].y = ctx->cfg->world_h - y - 0.2; // Y-flip, h/2
    ctx->chests[i].state = 0;
    ctx->chests[i].item_type = type;
    ctx->chests[i].show_key = 0;
    (*ctx->chest_count)++;
}

// P  x  y  (player top-left in meters)
static void load_tag_P(wl_ctx* ctx, const char* line){
    ctx->p_present = 1;
    if(ctx->spawn_set) return;
    double x,y; int n=sscanf(line+1,"%lf %lf",&x,&y);
    wl_check_fields(ctx,n,2,"P",line);
    *ctx->spawn_x = x + ctx->cfg->player_w/2.0;
    *ctx->spawn_y = ctx->cfg->world_h - y - ctx->cfg->player_h + ctx->cfg->player_h/2.0;
    ctx->spawn_set=1;
}

// D  x  y  w  h  color  [label]
// x,y = bottom-left in file space (y=0 ground, upward)
// color = hex 0xRRGGBBAA
// label optional — if starts with "teleporter" draws animated cage
static void load_tag_D(wl_ctx* ctx, const char* line){
    wl_check_cap(ctx,*ctx->decor_count,ctx->decor_max,"D",line);
    double x,y,w,h; unsigned int color; char rest[64]="";
    int n=sscanf(line+1,"%lf %lf %lf %lf %x %63[^\n]",&x,&y,&w,&h,&color,rest);
    if(n<5){ wl_check_fields(ctx,n,5,"D",line); }

    // strip leading spaces from rest
    char* label=rest;
    while(*label==' ') label++;

    int i=*ctx->decor_count;
    // Y-flip: file bottom-left -> physic center
    double py = ctx->cfg->world_h - y - h;
    ctx->decors[i].x     = x + w/2.0;
    ctx->decors[i].y     = py + h/2.0;
    ctx->decors[i].w     = w;
    ctx->decors[i].h     = h;
    ctx->decors[i].color = color;
    ctx->decors[i].is_teleporter = (strncmp(label,"teleporter",10)==0);
    // copy label (empty if none given or is "teleporter")
    if(ctx->decors[i].is_teleporter || label[0]=='\0')
        ctx->decors[i].label[0]='\0';
    else {
        strncpy(ctx->decors[i].label, label, 31);
        ctx->decors[i].label[31]='\0';
    }
    (*ctx->decor_count)++;
}

// O  x  y  w  h  friction  restitution  color
// x,y = bottom-left in file space (y=0 ground, upward)
// friction:    0.0=ice  1.0=full stop
// restitution: 0.0=no bounce  1.0=full bounce
// color = hex 0xRRGGBBAA
static void load_tag_O(wl_ctx* ctx, const char* line){
    wl_check_cap(ctx,*ctx->pobj_count,ctx->pobj_max,"O",line);
    double x,y,w,h,fr,rest; unsigned int color;
    int n=sscanf(line+1,"%lf %lf %lf %lf %lf %lf %x",&x,&y,&w,&h,&fr,&rest,&color);
    wl_check_fields(ctx,n,7,"O",line);
    int i=*ctx->pobj_count;
    double py = ctx->cfg->world_h - y - h;  // flip to physic top-left
    set_physic_base(&ctx->pobjs[i].base,
        x+w/2.0, py+h/2.0, 0,0,0,0, 1.0, fr,rest, 0,0,w,h);
    ctx->pobjs[i].active    = 1;
    ctx->pobjs[i].on_ground = 0;
    ctx->pobjs[i].color     = color;
    (*ctx->pobj_count)++;
}

//###############################################
// DEFAULT FILE
//###############################################
static void wl_write_default(const char* fp){
    FILE* f=fopen(fp,"w");
    if(!f){ fprintf(stderr,"[world_loader] ERROR: cannot write %s\n",fp); exit(1); }
    fprintf(f,
"# VERTICAL CLIMBER — world.txt\n"
"# All coordinates in METERS.\n"
"# Y=0 ground level, Y increases UPWARD.\n"
"# Tags:\n"
"#   V  key  value\n"
"#   T  x  y  w  h  type(solid|break)  coin_inside(0|1)\n"
"#   E  x  y  type(dasher|boss)  patrol_min  patrol_max\n"
"#   C  x  y\n"
"#   I  x  y  type\n"
"#   B  x  y  item_type\n"
"#   D  x  y  w  h  color(0xRRGGBBAA)  [label or 'teleporter']\n"
"#   O  x  y  w  h  friction  restitution  color(0xRRGGBBAA)\n"
"#   P  x  y  (player spawn, bottom-left)\n"
"# Reload mid-game: K\n"
"\n"
"# --- config ---\n"
"V px_per_m 100\n"
"V world_w 20\n"
"V world_h 20\n"
"V gravity 18\n"
"V move_speed 2.8\n"
"V move_accel 20\n"
"V move_friction 0.80\n"
"V jump_vy -7\n"
"V jump_boost_vy -9.5\n"
"V dash_speed 7\n"
"V dash_frames 6\n"
"V edge_grab_slide 0.5\n"
"V invincible_frames 40\n"
"V player_w 0.28\n"
"V player_h 0.36\n"
"V max_vx 10\n"
"V max_vy 20\n"
"\n"
"# --- spawn ---\n"
"P 1.0 1.2\n"
"\n"
"# --- walls + floor + ceiling ---\n"
"T -0.2 0 0.2 20 solid 0\n"
"T 20   0 0.2 20 solid 0\n"
"T 0 -0.4 20 0.4 solid 0\n"
"T 0 20   20 0.2 solid 0\n"
"\n"
"# --- ground ---\n"
"T 0 0 20 1.0 solid 0\n"
"\n"
"# --- platform (breakable) ---\n"
"T 4 4 4 0.4 break 0\n"
"\n"
"# --- platform (solid, upper) ---\n"
"T 10 8 6 0.4 solid 0\n"
"\n"
"# --- enemy ---\n"
"E 11 8.4 dasher 10 16\n"
"\n"
"# --- coin ---\n"
"C 3 1.8\n"
"\n"
"# --- item (speed boost) ---\n"
"I 7 1.5\n"
"\n"
"# --- chest ---\n"
"B 14 1.0 2\n"
"\n"
"# --- decor label ---\n"
"D 1 1.0 2.0 0.3 0xF5DEB3FF START\n"
"\n"
"# --- teleporter (win) ---\n"
"D 12 8.4 0.8 0.8 0x00FFFF44 teleporter\n"
"\n"
"# --- pushable box ---\n"
"O 6 1.0 0.4 0.4 0.3 0.2 0xAA6633FF\n"
    );
    fclose(f);
    printf("[world_loader] wrote default %s\n",fp);
}

//###############################################
// MAIN LOAD FUNCTION
// preserve_player=1: skip P tag (K reload)
// preserve_player=0: apply P tag (new game)
//###############################################
static void load_world(
    const char* filepath, int preserve_player,
    struct game_config*  cfg,
    struct terrain_data* terrains, int* tc, int tm,
    struct enemy_data*   enemies,  int* ec, int em,
    struct coin_data*    coins,    int* cc, int cm,
    struct item_data*    items,    int* ic, int im,
    struct decor_data*   decors,   int* dc, int dm,
    struct chest_data* chests_arr, int* chest_c, int chest_m,
    struct pobj_data*    pobjs,    int* oc, int om,
    double* spawn_x, double* spawn_y
){
    FILE* f=fopen(filepath,"r");
    if(!f){
        printf("[world_loader] %s not found, generating default.\n",filepath);
        wl_write_default(filepath);
        f=fopen(filepath,"r");
        if(!f){ fprintf(stderr,"[world_loader] ERROR: cannot open %s\n",filepath); exit(1); }
    }
    *cfg=game_config_default();
    *tc=0; *ec=0; *cc=0; *ic=0; *dc=0;
    *chest_c = 0; *oc = 0;
    wl_ctx ctx={0};
    ctx.filepath=filepath; ctx.cfg=cfg;
    ctx.terrains=terrains; ctx.terrain_count=tc; ctx.terrain_max=tm;
    ctx.enemies=enemies;   ctx.enemy_count=ec;   ctx.enemy_max=em;
    ctx.coins=coins;       ctx.coin_count=cc;     ctx.coin_max=cm;
    ctx.items=items;       ctx.item_count=ic;     ctx.item_max=im;
    ctx.decors=decors;     ctx.decor_count=dc;    ctx.decor_max=dm;
    ctx.spawn_x=spawn_x;   ctx.spawn_y=spawn_y;
    ctx.spawn_set=preserve_player;

    ctx.chests = chests_arr;
    ctx.chest_count = chest_c;
    ctx.chest_max = chest_m;

    ctx.pobjs       = pobjs;
    ctx.pobj_count  = oc;
    ctx.pobj_max    = om;

    char line[512];
    while(fgets(line,sizeof(line),f)){
        ctx.linenum++;
        int len=(int)strlen(line);
        while(len>0&&(line[len-1]=='\n'||line[len-1]=='\r'||line[len-1]==' '))
            line[--len]='\0';
        if(len==0||line[0]=='#') continue;
        switch(line[0]){
            case 'V': load_tag_V(&ctx,line); break;
            case 'T': load_tag_T(&ctx,line); break;
            case 'E': load_tag_E(&ctx,line); break;
            case 'C': load_tag_C(&ctx,line); break;
            case 'I': load_tag_I(&ctx,line); break;
            case 'B': load_tag_B(&ctx,line); break;
            case 'P': load_tag_P(&ctx,line); break;
            case 'D': load_tag_D(&ctx,line); break;
            case 'O': load_tag_O(&ctx,line); break;
            default:  wl_error(&ctx,"unknown tag (V T E C I P D O)",line);
        }
    }
    fclose(f);
    printf("[world_loader] %s — T:%d E:%d C:%d I:%d D:%d O:%d P:%s\n",
           filepath,*tc,*ec,*cc,*ic,*dc,*oc,preserve_player?"preserved":"loaded");

    // --- AUTO REPAIR: insert missing fields at correct positions ---
    struct game_config* c = cfg;
    int need_repair =
        !ctx.v_px_per_m || !ctx.v_world_w    || !ctx.v_world_h         ||
        !ctx.v_gravity   || !ctx.v_move_speed || !ctx.v_move_accel      ||
        !ctx.v_move_friction || !ctx.v_jump_vy || !ctx.v_jump_boost_vy  ||
        !ctx.v_dash_speed || !ctx.v_dash_frames || !ctx.v_edge_grab_slide||
        !ctx.v_invincible_frames || !ctx.v_player_w || !ctx.v_player_h  ||
        !ctx.v_max_vx || !ctx.v_max_vy || !ctx.p_present;

    if(need_repair){
        // read entire file into memory
        FILE* rf = fopen(filepath, "r");
        if(!rf){
            fprintf(stderr,"[world_loader] WARNING: cannot repair %s\n",filepath);
            return;
        }
        fseek(rf, 0, SEEK_END);
        long fsz = ftell(rf);
        rewind(rf);
        char* fbuf = (char*)malloc(fsz + 4096); // extra room for inserts
        if(!fbuf){ fclose(rf); fprintf(stderr,"[world_loader] WARNING: out of memory for repair\n"); return; }
        fread(fbuf, 1, fsz, rf);
        fbuf[fsz] = '\0';
        fclose(rf);

        // output buffer (generous size)
        char* out = (char*)malloc(fsz + 8192);
        if(!out){ free(fbuf); return; }
        out[0] = '\0';
        char* op = out;

        // insertion strategy:
        //   missing V keys -> inserted after last existing V line
        //   missing P      -> inserted after last existing P line,
        //                     or after last V line if no P exists
        // We walk line by line, track last_v_pos and last_p_pos in output,
        // then do a two-pass: first pass builds output normally,
        // second pass we insert missing lines at tracked positions.
        //
        // Simpler single-pass: after writing each V line, check if the
        // *next* line is NOT a V line and we still have missing V keys ->
        // insert them now. Same for P.

        int last_was_v = 0, last_was_p = 0;
        char* src = fbuf;

        while(*src){
            // find end of line
            char* eol = src;
            while(*eol && *eol != '\n') eol++;
            int llen = (int)(eol - src);
            char lbuf[512];
            if(llen >= 511) llen = 511;
            strncpy(lbuf, src, llen);
            lbuf[llen] = '\0';

            int is_v = (lbuf[0]=='V');
            int is_p = (lbuf[0]=='P');

            // detect transition away from V block -> insert missing V keys
            if(last_was_v && !is_v){
                if(!ctx.v_px_per_m)          { op+=sprintf(op,"V px_per_m %g\n",          c->px_per_m);          ctx.v_px_per_m=1; }
                if(!ctx.v_world_w)           { op+=sprintf(op,"V world_w %g\n",           c->world_w);           ctx.v_world_w=1; }
                if(!ctx.v_world_h)           { op+=sprintf(op,"V world_h %g\n",           c->world_h);           ctx.v_world_h=1; }
                if(!ctx.v_gravity)           { op+=sprintf(op,"V gravity %g\n",           c->gravity);           ctx.v_gravity=1; }
                if(!ctx.v_move_speed)        { op+=sprintf(op,"V move_speed %g\n",        c->move_speed);        ctx.v_move_speed=1; }
                if(!ctx.v_move_accel)        { op+=sprintf(op,"V move_accel %g\n",        c->move_accel);        ctx.v_move_accel=1; }
                if(!ctx.v_move_friction)     { op+=sprintf(op,"V move_friction %g\n",     c->move_friction);     ctx.v_move_friction=1; }
                if(!ctx.v_jump_vy)           { op+=sprintf(op,"V jump_vy %g\n",           c->jump_vy);           ctx.v_jump_vy=1; }
                if(!ctx.v_jump_boost_vy)     { op+=sprintf(op,"V jump_boost_vy %g\n",     c->jump_boost_vy);     ctx.v_jump_boost_vy=1; }
                if(!ctx.v_dash_speed)        { op+=sprintf(op,"V dash_speed %g\n",        c->dash_speed);        ctx.v_dash_speed=1; }
                if(!ctx.v_dash_frames)       { op+=sprintf(op,"V dash_frames %d\n",       c->dash_frames);       ctx.v_dash_frames=1; }
                if(!ctx.v_edge_grab_slide)   { op+=sprintf(op,"V edge_grab_slide %g\n",   c->edge_grab_slide);   ctx.v_edge_grab_slide=1; }
                if(!ctx.v_invincible_frames) { op+=sprintf(op,"V invincible_frames %d\n", c->invincible_frames); ctx.v_invincible_frames=1; }
                if(!ctx.v_player_w)          { op+=sprintf(op,"V player_w %g\n",          c->player_w);          ctx.v_player_w=1; }
                if(!ctx.v_player_h)          { op+=sprintf(op,"V player_h %g\n",          c->player_h);          ctx.v_player_h=1; }
                if(!ctx.v_max_vx)            { op+=sprintf(op,"V max_vx %g\n",            c->max_vx);            ctx.v_max_vx=1; }
                if(!ctx.v_max_vy)            { op+=sprintf(op,"V max_vy %g\n",            c->max_vy);            ctx.v_max_vy=1; }
            }

            // detect transition away from P block -> insert missing P
            if(last_was_p && !is_p && !ctx.p_present){
                op+=sprintf(op,"P %g %g\n",
                    *spawn_x - c->player_w/2.0,
                    c->world_h - *spawn_y - c->player_h/2.0);
                ctx.p_present=1;
            }

            // write current line
            memcpy(op, src, llen);
            op += llen;
            if(*eol == '\n'){ *op++ = '\n'; }

            last_was_v = is_v;
            last_was_p = is_p;
            src = (*eol == '\n') ? eol+1 : eol;
        }

        // if V or P block was at end of file (no trailing non-V/P line)
        if(!ctx.v_px_per_m||!ctx.v_world_w||!ctx.v_world_h||!ctx.v_gravity||
           !ctx.v_move_speed||!ctx.v_move_accel||!ctx.v_move_friction||
           !ctx.v_jump_vy||!ctx.v_jump_boost_vy||!ctx.v_dash_speed||
           !ctx.v_dash_frames||!ctx.v_edge_grab_slide||!ctx.v_invincible_frames||
           !ctx.v_player_w||!ctx.v_player_h||!ctx.v_max_vx||!ctx.v_max_vy){
            if(!ctx.v_px_per_m)          op+=sprintf(op,"V px_per_m %g\n",          c->px_per_m);
            if(!ctx.v_world_w)           op+=sprintf(op,"V world_w %g\n",           c->world_w);
            if(!ctx.v_world_h)           op+=sprintf(op,"V world_h %g\n",           c->world_h);
            if(!ctx.v_gravity)           op+=sprintf(op,"V gravity %g\n",           c->gravity);
            if(!ctx.v_move_speed)        op+=sprintf(op,"V move_speed %g\n",        c->move_speed);
            if(!ctx.v_move_accel)        op+=sprintf(op,"V move_accel %g\n",        c->move_accel);
            if(!ctx.v_move_friction)     op+=sprintf(op,"V move_friction %g\n",     c->move_friction);
            if(!ctx.v_jump_vy)           op+=sprintf(op,"V jump_vy %g\n",           c->jump_vy);
            if(!ctx.v_jump_boost_vy)     op+=sprintf(op,"V jump_boost_vy %g\n",     c->jump_boost_vy);
            if(!ctx.v_dash_speed)        op+=sprintf(op,"V dash_speed %g\n",        c->dash_speed);
            if(!ctx.v_dash_frames)       op+=sprintf(op,"V dash_frames %d\n",       c->dash_frames);
            if(!ctx.v_edge_grab_slide)   op+=sprintf(op,"V edge_grab_slide %g\n",   c->edge_grab_slide);
            if(!ctx.v_invincible_frames) op+=sprintf(op,"V invincible_frames %d\n", c->invincible_frames);
            if(!ctx.v_player_w)          op+=sprintf(op,"V player_w %g\n",          c->player_w);
            if(!ctx.v_player_h)          op+=sprintf(op,"V player_h %g\n",          c->player_h);
            if(!ctx.v_max_vx)            op+=sprintf(op,"V max_vx %g\n",            c->max_vx);
            if(!ctx.v_max_vy)            op+=sprintf(op,"V max_vy %g\n",            c->max_vy);
        }
        if(!ctx.p_present){
            op+=sprintf(op,"P %g %g\n",
                *spawn_x - c->player_w/2.0,
                c->world_h - *spawn_y - c->player_h/2.0);
        }
        *op = '\0';

        // write back
        FILE* wf = fopen(filepath, "w");
        if(!wf){
            fprintf(stderr,"[world_loader] WARNING: cannot write repaired %s\n",filepath);
        } else {
            fwrite(out, 1, strlen(out), wf);
            fclose(wf);
            printf("[world_loader] repaired missing fields in %s\n", filepath);
        }
        free(fbuf);
        free(out);
    }
}

#endif
