#ifndef INIT_H
#define INIT_H
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "10_data.h"
#include "external_lib/physic.h"

//###############################################
// HELPERS
//###############################################
static int _ti = 0;
static int _ci = 0;

static void add_terrain(double x,double y,double w,double h,
                        int type,int hp,int coin_inside){
    if(_ti>=TERRAIN_COUNT) return;
    set_physic_base(&terrains[_ti].base,
        x+w/2.0,y+h/2.0, 0,0,0,0,-1,0.4,0.0,0,0,w,h);
    terrains[_ti].type=type; terrains[_ti].hp=hp;
    terrains[_ti].broken=0; terrains[_ti].broken_timer=0;
    terrains[_ti].coin_inside=coin_inside;
    _ti++;
}

static void S(double x,double y,double w,double h){
    add_terrain(x,y,w,h,TERRAIN_SOLID,-1,0);
}
static void B(double x,double y,double w,double h,int c){
    add_terrain(x,y,w,h,TERRAIN_BREAK,1,c);
}
static void stair(double x,double y,double sw,double sh,int n,double dx,double dy){
    for(int i=0;i<n;i++) S(x+i*dx,y+i*dy,sw,sh);
}
static void face(double x,double y,double h){ S(x,y,50,h); }
static void hang(double x,double y,double w){ S(x,y,w,18); }
static void boulder(double x,double y){
    S(x,y,90,70); S(x+80,y+20,65,50); S(x+15,y-35,55,40);
}
static void ceiling_chunk(double x,double y,double w,double h){ S(x,y,w,h); }
static void chimney(double x,double y,double gap,double h){
    face(x,y,h); face(x+gap+50,y,h);
}

static void add_coin(double x,double y){
    if(_ci>=COIN_COUNT) return;
    coins[_ci].x=x; coins[_ci].y=y; coins[_ci].collected=0; _ci++;
}

static void reset_player(){
    set_physic_base(&player.base,
        player.respawn_x,player.respawn_y,
        0,0,0,GRAVITY,60,0.4,0.0,0,0,28,36);
    player.on_ground=0; player.on_wall=0;
    player.edge_grab=0; player.grab_wall_dir=0;
    player.jump_count=0; player.dash_ready=1;
    player.dashing=0; player.dash_dir=1;
    player.invincible=0; player.god_mode=0;
    player.last_move_dir=1; player.jump_boost_timer=0;
}

//###############################################
// TERRAIN LAYOUT v5
// Evenly filled 2000x14000 with all variants.
// Vertical gap rule: <= 110px between reachable tops.
// Variants distributed: stair, face, chimney, hang,
//   boulder, ceiling_chunk, B (break), S (plain).
// No explicit routes — player discovers organically.
//
// Layout rhythm per ~700px vertical band:
//   - Wide base ledges + boulders (low zones)
//   - Staircase faces + ceiling chunks (mid zones)
//   - Chimneys + hangs + tight gaps (high zones)
//   - All variants mixed throughout for variety
//###############################################
static void init_terrain(){
    _ti=0;

    // BOUNDARIES
    S(-20,0,20,WORLD_H);
    S(WORLD_W,0,20,WORLD_H);
    S(0,WORLD_H,WORLD_W,60);   // solid bottom floor
    S(0,0,WORLD_W,20);         // top cap

    //--------------------------------------------------
    // BAND 0: y 13300-14000  BASE
    //--------------------------------------------------
    S(0,    13880, 2000, 120);   // main ground
    S(0,    13750,  340,  80);
    S(1660, 13750,  340,  80);
    boulder(860, 13760);
    boulder(1060,13740);
    face(0,  13200, 560);
    face(1950,13200,560);
    S(55,   13680,  200,  65);
    S(280,  13580,  220,  65);
    S(500,  13500,  200,  65);
    S(1250, 13680,  200,  65);
    S(1480, 13580,  220,  65);
    S(1700, 13500,  200,  65);
    ceiling_chunk(400,13420,300,45);
    ceiling_chunk(1300,13420,300,45);
    S(440,  13360,  260,  65);
    S(1300, 13360,  260,  65);
    B(700,  13380,  160,  55, 1);
    B(1140, 13380,  160,  55, 0);

    //--------------------------------------------------
    // BAND 1: y 12600-13300
    //--------------------------------------------------
    stair(55, 13270, 160,60, 5, 50,-90);   // left stair up
    stair(1785,13270,160,60, 5,-50,-90);   // right stair (mirror)
    ceiling_chunk(350,13050,320,45);
    ceiling_chunk(1330,13050,320,45);
    S(380,  12990,  280,  65);
    S(1340, 12990,  280,  65);
    boulder(680, 13000);
    boulder(1100,12980);
    hang(400, 12880, 260);
    hang(1340,12880, 260);
    S(420,  12840,  240,  65);
    S(1340, 12840,  240,  65);
    S(800,  12860,  400,  65);
    ceiling_chunk(760,12760,480,40);
    stair(820,12730,150,55, 4, 40,-85);
    S(55,   12820,  200,  65);
    S(1745, 12820,  200,  65);
    face(0,  12600, 360);
    face(1950,12600,360);
    S(55,   12770,  190,  65);
    S(1755, 12770,  190,  65);
    B(300,  12700,  150,  50, 1);
    B(1550, 12700,  150,  50, 0);

    //--------------------------------------------------
    // BAND 2: y 11900-12600
    //--------------------------------------------------
    chimney(0,  12250,220,400);    // left chimney
    chimney(1730,12250,220,400);   // right chimney
    S(55,   12230,  240,  55);     // chimney floor L
    S(1705, 12230,  240,  55);     // chimney floor R
    hang(55, 12050, 240);
    hang(1705,12050,240);
    S(60,   12010,  220,  60);
    S(1720, 12010,  220,  60);
    ceiling_chunk(330,11990,380,42);
    ceiling_chunk(1290,11990,380,42);
    S(360,  11930,  320,  65);
    S(1320, 11930,  320,  65);
    S(760,  11950,  480,  65);
    boulder(820,11870);
    ceiling_chunk(740,11840,520,40);
    stair(780,11810,140,52, 5, 36,-82);
    S(55,   11920,  200,  65);
    S(1745, 11920,  200,  65);
    B(550,  11860,  150,  50, 1);
    B(1300, 11860,  150,  50, 0);

    //--------------------------------------------------
    // BAND 3: y 11200-11900
    //--------------------------------------------------
    face(0,  11200,420);
    face(1950,11200,420);
    stair(55, 11870, 150,58, 5, 42,-82);
    stair(1795,11870,150,58, 5,-42,-82);
    S(310,  11770,  300,  65);
    S(1390, 11770,  300,  65);
    hang(310,11660, 300);
    hang(1390,11660,300);
    S(330,  11620,  260,  65);
    S(1410, 11620,  260,  65);
    ceiling_chunk(680,11620,640,42);
    S(700,  11560,  600,  65);
    boulder(760,11480);
    boulder(1100,11460);
    ceiling_chunk(720,11440,560,40);
    stair(740,11410,135,50, 5, 32,-78);
    S(55,   11530,  195,  65);
    S(1750, 11530,  195,  65);
    B(430,  11460,  145,  48, 0);
    B(1425, 11460,  145,  48, 1);

    //--------------------------------------------------
    // BAND 4: y 10500-11200
    //--------------------------------------------------
    chimney(0,  10700,200,380);
    chimney(1750,10700,200,380);
    S(55,   10680,  230,  55);
    S(1715, 10680,  230,  55);
    hang(55, 10510, 230);
    hang(1715,10510,230);
    S(60,   10470,  215,  60);
    S(1725, 10470,  215,  60);
    ceiling_chunk(340,10500,380,42);
    ceiling_chunk(1280,10500,380,42);
    S(360,  10440,  340,  65);
    S(1300, 10440,  340,  65);
    S(780,  10460,  440,  65);
    boulder(840,10380);
    ceiling_chunk(760,10360,480,40);
    stair(800,10330,130,48, 5, 30,-75);
    S(55,   10440,  200,  65);
    S(1745, 10440,  200,  65);
    B(520,  10370,  140,  48, 1);
    B(1340, 10370,  140,  48, 0);

    //--------------------------------------------------
    // BAND 5: y 9800-10500
    //--------------------------------------------------
    face(0,  9800,420);
    face(1950,9800,420);
    stair(55, 10470, 145,55, 5, 38,-76);
    stair(1800,10470,145,55, 5,-38,-76);
    S(330,  10360,  280,  65);
    S(1390, 10360,  280,  65);
    hang(330,10250, 280);
    hang(1390,10250,280);
    S(350,  10210,  240,  65);
    S(1410, 10210,  240,  65);
    ceiling_chunk(690,10220,620,42);
    S(710,  10160,  580,  65);
    boulder(780,10080);
    boulder(1100,10060);
    ceiling_chunk(710,10040,580,40);
    stair(730,10010,128,47, 5, 28,-72);
    S(55,   10220,  195,  65);
    S(1750, 10220,  195,  65);
    B(450,  10140,  140,  47, 0);
    B(1410, 10140,  140,  47, 1);

    //--------------------------------------------------
    // BAND 6: y 9100-9800
    //--------------------------------------------------
    chimney(0,  9300,190,360);
    chimney(1760,9300,190,360);
    S(55,   9280,  225,  55);
    S(1725, 9280,  225,  55);
    hang(55, 9110, 225);
    hang(1725,9110,225);
    S(60,   9070,  210,  60);
    S(1730, 9070,  210,  60);
    ceiling_chunk(350,9100,360,42);
    ceiling_chunk(1290,9100,360,42);
    S(370,  9040,  320,  65);
    S(1310, 9040,  320,  65);
    S(790,  9060,  420,  65);
    boulder(850,8980);
    ceiling_chunk(770,8960,460,40);
    stair(810,8930,125,46, 5, 26,-70);
    S(55,   9040,  200,  65);
    S(1745, 9040,  200,  65);
    B(530,  8960,  135,  46, 1);
    B(1335, 8960,  135,  46, 0);

    //--------------------------------------------------
    // BAND 7: y 8400-9100
    //--------------------------------------------------
    face(0,  8400,420);
    face(1950,8400,420);
    stair(55, 9070, 138,53, 5, 34,-72);
    stair(1807,9070,138,53, 5,-34,-72);
    S(360,  8960,  260,  65);
    S(1380, 8960,  260,  65);
    hang(360,8850, 260);
    hang(1380,8850,260);
    S(380,  8810,  220,  65);
    S(1400, 8810,  220,  65);
    ceiling_chunk(700,8820,600,42);
    S(720,  8760,  560,  65);
    boulder(790,8680);
    boulder(1110,8660);
    ceiling_chunk(720,8640,560,40);
    stair(740,8610,122,45, 5, 24,-68);
    S(55,   8820,  195,  65);
    S(1750, 8820,  195,  65);
    B(460,  8740,  135,  45, 0);
    B(1405, 8740,  135,  45, 1);

    //--------------------------------------------------
    // BAND 8: y 7700-8400
    //--------------------------------------------------
    chimney(0,  7900,185,350);
    chimney(1765,7900,185,350);
    S(55,   7880,  220,  55);
    S(1730, 7880,  220,  55);
    hang(55, 7720, 220);
    hang(1730,7720,220);
    S(60,   7680,  205,  60);
    S(1735, 7680,  205,  60);
    ceiling_chunk(360,7720,340,42);
    ceiling_chunk(1300,7720,340,42);
    S(380,  7660,  300,  65);
    S(1320, 7660,  300,  65);
    S(800,  7680,  400,  65);
    boulder(860,7600);
    ceiling_chunk(780,7580,440,40);
    stair(820,7550,120,44, 5, 22,-66);
    S(55,   7660,  200,  65);
    S(1745, 7660,  200,  65);
    B(540,  7580,  130,  44, 1);
    B(1330, 7580,  130,  44, 0);

    //--------------------------------------------------
    // BAND 9: y 7000-7700
    //--------------------------------------------------
    face(0,  7000,420);
    face(1950,7000,420);
    stair(55, 7670, 130,51, 5, 30,-68);
    stair(1815,7670,130,51, 5,-30,-68);
    S(380,  7560,  240,  65);
    S(1380, 7560,  240,  65);
    hang(380,7450, 240);
    hang(1380,7450,240);
    S(400,  7410,  200,  65);
    S(1400, 7410,  200,  65);
    ceiling_chunk(710,7420,580,42);
    S(730,  7360,  540,  65);
    boulder(800,7280);
    boulder(1100,7260);
    ceiling_chunk(730,7240,540,40);
    stair(750,7210,118,43, 5, 20,-64);
    S(55,   7420,  195,  65);
    S(1750, 7420,  195,  65);
    B(470,  7340,  130,  43, 0);
    B(1400, 7340,  130,  43, 1);

    //--------------------------------------------------
    // BAND 10: y 6300-7000
    //--------------------------------------------------
    chimney(0,  6500,180,340);
    chimney(1770,6500,180,340);
    S(55,   6480,  215,  55);
    S(1735, 6480,  215,  55);
    hang(55, 6330, 215);
    hang(1735,6330,215);
    S(60,   6290,  200,  60);
    S(1740, 6290,  200,  60);
    ceiling_chunk(370,6340,320,42);
    ceiling_chunk(1310,6340,320,42);
    S(390,  6280,  280,  65);
    S(1330, 6280,  280,  65);
    S(810,  6300,  380,  65);
    boulder(870,6220);
    ceiling_chunk(790,6200,420,40);
    stair(830,6170,115,42, 5, 18,-62);
    S(55,   6280,  200,  65);
    S(1745, 6280,  200,  65);
    B(550,  6200,  125,  42, 1);
    B(1325, 6200,  125,  42, 0);

    //--------------------------------------------------
    // BAND 11: y 5600-6300
    //--------------------------------------------------
    face(0,  5600,420);
    face(1950,5600,420);
    stair(55, 6270, 125,49, 5, 26,-64);
    stair(1820,6270,125,49, 5,-26,-64);
    S(400,  6160,  220,  65);
    S(1380, 6160,  220,  65);
    hang(400,6050, 220);
    hang(1380,6050,220);
    S(420,  6010,  180,  65);
    S(1400, 6010,  180,  65);
    ceiling_chunk(720,6020,560,42);
    S(740,  5960,  520,  65);
    boulder(810,5880);
    boulder(1090,5860);
    ceiling_chunk(740,5840,520,40);
    stair(760,5810,113,41, 5, 16,-60);
    S(55,   6020,  195,  65);
    S(1750, 6020,  195,  65);
    B(480,  5940,  125,  41, 0);
    B(1395, 5940,  125,  41, 1);

    //--------------------------------------------------
    // BAND 12: y 4900-5600
    //--------------------------------------------------
    chimney(0,  5100,175,330);
    chimney(1775,5100,175,330);
    S(55,   5080,  210,  55);
    S(1740, 5080,  210,  55);
    hang(55, 4940, 210);
    hang(1740,4940,210);
    S(60,   4900,  195,  60);
    S(1745, 4900,  195,  60);
    ceiling_chunk(380,4960,300,42);
    ceiling_chunk(1320,4960,300,42);
    S(400,  4900,  260,  65);
    S(1340, 4900,  260,  65);
    S(820,  4920,  360,  65);
    boulder(880,4840);
    ceiling_chunk(800,4820,400,40);
    stair(840,4790,110,40, 5, 14,-58);
    S(55,   4900,  200,  65);
    S(1745, 4900,  200,  65);
    B(560,  4820,  120,  40, 1);
    B(1320, 4820,  120,  40, 0);

    //--------------------------------------------------
    // BAND 13: y 4200-4900
    //--------------------------------------------------
    face(0,  4200,420);
    face(1950,4200,420);
    stair(55, 4870, 120,47, 5, 22,-60);
    stair(1825,4870,120,47, 5,-22,-60);
    S(420,  4760,  200,  65);
    S(1380, 4760,  200,  65);
    hang(420,4650, 200);
    hang(1380,4650,200);
    S(440,  4610,  160,  65);
    S(1400, 4610,  160,  65);
    ceiling_chunk(730,4620,540,42);
    S(750,  4560,  500,  65);
    boulder(820,4480);
    boulder(1080,4460);
    ceiling_chunk(750,4440,500,40);
    stair(770,4410,110,39, 5, 12,-56);
    S(55,   4620,  195,  65);
    S(1750, 4620,  195,  65);
    B(490,  4540,  120,  39, 0);
    B(1390, 4540,  120,  39, 1);

    //--------------------------------------------------
    // BAND 14: y 3500-4200
    //--------------------------------------------------
    chimney(0,  3700,170,320);
    chimney(1780,3700,170,320);
    S(55,   3680,  205,  55);
    S(1745, 3680,  205,  55);
    hang(55, 3550, 205);
    hang(1745,3550,205);
    S(60,   3510,  190,  60);
    S(1750, 3510,  190,  60);
    ceiling_chunk(390,3560,280,42);
    ceiling_chunk(1330,3560,280,42);
    S(410,  3500,  240,  65);
    S(1350, 3500,  240,  65);
    S(830,  3520,  340,  65);
    boulder(890,3440);
    ceiling_chunk(810,3420,380,40);
    stair(850,3390,108,38, 5, 10,-54);
    S(55,   3500,  200,  65);
    S(1745, 3500,  200,  65);
    B(570,  3420,  115,  38, 1);
    B(1315, 3420,  115,  38, 0);

    //--------------------------------------------------
    // BAND 15: y 2800-3500
    //--------------------------------------------------
    face(0,  2800,420);
    face(1950,2800,420);
    stair(55, 3470, 115,45, 5, 18,-56);
    stair(1830,3470,115,45, 5,-18,-56);
    S(440,  3360,  180,  65);
    S(1380, 3360,  180,  65);
    hang(440,3250, 180);
    hang(1380,3250,180);
    S(460,  3210,  140,  65);
    S(1400, 3210,  140,  65);
    ceiling_chunk(740,3220,520,42);
    S(760,  3160,  480,  65);
    boulder(830,3080);
    boulder(1070,3060);
    ceiling_chunk(760,3040,480,40);
    stair(780,3010,106,37, 5, 8,-52);
    S(55,   3220,  195,  65);
    S(1750, 3220,  195,  65);
    B(500,  3140,  115,  37, 0);
    B(1385, 3140,  115,  37, 1);

    //--------------------------------------------------
    // BAND 16: y 2100-2800
    //--------------------------------------------------
    chimney(0,  2300,165,310);
    chimney(1785,2300,165,310);
    S(55,   2280,  200,  55);
    S(1750, 2280,  200,  55);
    hang(55, 2160, 200);
    hang(1750,2160,200);
    S(60,   2120,  185,  60);
    S(1755, 2120,  185,  60);
    ceiling_chunk(400,2180,260,42);
    ceiling_chunk(1340,2180,260,42);
    S(420,  2120,  220,  65);
    S(1360, 2120,  220,  65);
    S(840,  2140,  320,  65);
    boulder(900,2060);
    ceiling_chunk(820,2040,360,40);
    stair(860,2010,104,36, 5, 6,-50);
    S(55,   2120,  200,  65);
    S(1745, 2120,  200,  65);
    B(580,  2040,  110,  36, 1);
    B(1310, 2040,  110,  36, 0);

    //--------------------------------------------------
    // BAND 17: y 1400-2100  (SUMMIT APPROACH)
    //--------------------------------------------------
    face(0,  1400,420);
    face(1950,1400,420);
    stair(55, 2070, 110,43, 5, 14,-52);
    stair(1835,2070,110,43, 5,-14,-52);
    S(460,  1960,  160,  65);
    S(1380, 1960,  160,  65);
    hang(460,1850, 160);
    hang(1380,1850,160);
    S(480,  1810,  120,  65);
    S(1400, 1810,  120,  65);
    // wide approach merging both sides
    S(300,  1750, 1400,  55);
    ceiling_chunk(280,1640,1440,42);
    S(320,  1580, 1360,  55);
    S(340,  1450, 1320,  55);   // boss floor

    //--------------------------------------------------
    // BAND 18: y 700-1400  (NEAR SUMMIT)
    //--------------------------------------------------
    S(360,  1320, 1280,  55);
    ceiling_chunk(340,1210,1320,40);
    S(380,  1150, 1240,  55);
    S(400,  1020, 1200,  55);
    S(420,   890, 1160,  50);
    ceiling_chunk(400, 780,1200,38);
    S(440,   720, 1120,  50);
    S(460,   590, 1080,  45);

    //--------------------------------------------------
    // BAND 19: y 0-700  (SUMMIT)
    //--------------------------------------------------
    S(480,   470, 1040,  45);
    S(520,   360,  960,  40);
    S(560,   300,  880,  30);   // WIN PLATFORM (teleporter sits here)
    // small raised rocks at summit edges for decor
    boulder(560, 260);
    boulder(1270,260);

    terrain_count_actual = _ti;
}

//###############################################
// COINS
//###############################################
static void init_coins(){
    _ci=0;
    add_coin(240, 13700); add_coin(1760,13700);
    add_coin(480, 13460); add_coin(1520,13460);
    add_coin(120, 12800); add_coin(1880,12800);
    add_coin(360, 12620); add_coin(1640,12620);
    add_coin(100, 11880); add_coin(1900,11880);
    add_coin(340, 11620); add_coin(1660,11620);
    add_coin(90,  10940); add_coin(1910,10940);
    add_coin(320, 10420); add_coin(1680,10420);
    add_coin(80,   9280); add_coin(1920, 9280);
    add_coin(300,  9040); add_coin(1700, 9040);
    add_coin(70,   7880); add_coin(1930, 7880);
    add_coin(280,  7660); add_coin(1720, 7660);
    add_coin(60,   6480); add_coin(1940, 6480);
    add_coin(260,  6280); add_coin(1740, 6280);
    add_coin(700,   420); add_coin(1300,  420);
}

//###############################################
// ITEMS
//###############################################
static void init_items(){
    items[0].x=1000; items[0].y=5940;
    items[0].active=1; items[0].respawn_timer=0;

    items[1].x=1000; items[1].y=3140;
    items[1].active=1; items[1].respawn_timer=0;
}

//###############################################
// ENEMIES
//###############################################
static void init_enemies(){
    // pair of enemies per major band
    set_physic_base(&enemies[0].base,200,10420,0,0,0,0,-1,0.4,0,0,0,32,32);
    enemies[0].type=ENEMY_DASHER; enemies[0].active=1; enemies[0].stun_timer=0;
    enemies[0].patrol_dir=1; enemies[0].patrol_timer=60;
    enemies[0].patrol_x_min=55; enemies[0].patrol_x_max=260;
    enemies[0].patrol_y=10420; enemies[0].dash_timer=100;
    enemies[0].dashing=0; enemies[0].dash_vx=320; enemies[0].hp=1;

    set_physic_base(&enemies[1].base,1800,10420,0,0,0,0,-1,0.4,0,0,0,32,32);
    enemies[1].type=ENEMY_DASHER; enemies[1].active=1; enemies[1].stun_timer=0;
    enemies[1].patrol_dir=-1; enemies[1].patrol_timer=60;
    enemies[1].patrol_x_min=1740; enemies[1].patrol_x_max=1940;
    enemies[1].patrol_y=10420; enemies[1].dash_timer=100;
    enemies[1].dashing=0; enemies[1].dash_vx=320; enemies[1].hp=1;

    set_physic_base(&enemies[2].base,200,7660,0,0,0,0,-1,0.4,0,0,0,32,32);
    enemies[2].type=ENEMY_DASHER; enemies[2].active=1; enemies[2].stun_timer=0;
    enemies[2].patrol_dir=1; enemies[2].patrol_timer=65;
    enemies[2].patrol_x_min=55; enemies[2].patrol_x_max=250;
    enemies[2].patrol_y=7660; enemies[2].dash_timer=85;
    enemies[2].dashing=0; enemies[2].dash_vx=360; enemies[2].hp=1;

    set_physic_base(&enemies[3].base,1800,7660,0,0,0,0,-1,0.4,0,0,0,32,32);
    enemies[3].type=ENEMY_DASHER; enemies[3].active=1; enemies[3].stun_timer=0;
    enemies[3].patrol_dir=-1; enemies[3].patrol_timer=65;
    enemies[3].patrol_x_min=1750; enemies[3].patrol_x_max=1945;
    enemies[3].patrol_y=7660; enemies[3].dash_timer=85;
    enemies[3].dashing=0; enemies[3].dash_vx=360; enemies[3].hp=1;

    set_physic_base(&enemies[4].base,200,4900,0,0,0,0,-1,0.4,0,0,0,32,32);
    enemies[4].type=ENEMY_DASHER; enemies[4].active=1; enemies[4].stun_timer=0;
    enemies[4].patrol_dir=1; enemies[4].patrol_timer=70;
    enemies[4].patrol_x_min=55; enemies[4].patrol_x_max=245;
    enemies[4].patrol_y=4900; enemies[4].dash_timer=70;
    enemies[4].dashing=0; enemies[4].dash_vx=390; enemies[4].hp=1;

    set_physic_base(&enemies[5].base,1800,4900,0,0,0,0,-1,0.4,0,0,0,32,32);
    enemies[5].type=ENEMY_DASHER; enemies[5].active=1; enemies[5].stun_timer=0;
    enemies[5].patrol_dir=-1; enemies[5].patrol_timer=70;
    enemies[5].patrol_x_min=1755; enemies[5].patrol_x_max=1945;
    enemies[5].patrol_y=4900; enemies[5].dash_timer=70;
    enemies[5].dashing=0; enemies[5].dash_vx=390; enemies[5].hp=1;

    // BOSS at summit approach
    set_physic_base(&enemies[6].base,1000,1415,0,0,0,0,-1,0.4,0,0,0,64,64);
    enemies[6].type=ENEMY_BOSS; enemies[6].active=1; enemies[6].stun_timer=0;
    enemies[6].patrol_dir=1; enemies[6].patrol_timer=50;
    enemies[6].patrol_x_min=380; enemies[6].patrol_x_max=1620;
    enemies[6].patrol_y=1415; enemies[6].dash_timer=45;
    enemies[6].dashing=0; enemies[6].dash_vx=520; enemies[6].hp=5;
}

//###############################################
// RESET
//###############################################
void reset_game(){
    for(int i=0;i<COIN_COUNT;i++) coins[i].collected=0;
    init_items();
    init_enemies();
    for(int i=0;i<terrain_count_actual;i++){
        if(terrains[i].type==TERRAIN_BREAK){
            terrains[i].broken=0;
            terrains[i].broken_timer=0;
            terrains[i].hp=1;
        }
    }
    player.respawn_x=1000;
    player.respawn_y=13800;
    player.score=0;
    reset_player();
    camera.x=player.base.x-SCREEN_W/2.0;
    camera.y=player.base.y-SCREEN_H/2.0;
    if(camera.x<0) camera.x=0;
    if(camera.y<0) camera.y=0;
    if(camera.x>WORLD_W-SCREEN_W) camera.x=WORLD_W-SCREEN_W;
    if(camera.y>WORLD_H-SCREEN_H) camera.y=WORLD_H-SCREEN_H;
}

//###############################################
// INIT
//###############################################
int init(){
    init_graphic_lib();
    init_window(&window,SCREEN_W,SCREEN_H,"Vertical Climber");
    render_flag=0;
    reset_timer(&tps_timer,1.0/20.0);
    load_img(&window,&font.data,"resource/font_ASCII.png");
    set_font(&font,5,1);
    init_terrain();
    init_coins();
    game_state=STATE_MENU;
    menu_sub_state=0;
    player.respawn_x=1000;
    player.respawn_y=13800;
    player.score=0;
    reset_game();
    return 1;
}
#endif
