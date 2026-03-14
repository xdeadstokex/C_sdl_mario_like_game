# C_sdl_mario_like_game
DES:
```txt
Vertical Climber is a 2D vertical platformer where you climb a massive mountain
using jumps, wall grabs, and dashes. Navigate cliffs, chimneys, and hanging rocks
while collecting coins, avoiding enemies, and using temporary jump boosts.
Reach the summit and enter the teleporter to win.

How to play

A / D — move left / right

Space — jump (or wall jump)

Shift — dash

Climb by grabbing walls (pressing A D against wall),
chain jumps and dashes to reach higher terrain.
```

DEPENDENCY
```txt
gcc:        (find it yourself in window, mingw32 is normally used)
SDL2:        https://github.com/libsdl-org/SDL/releases/tag/release-2.32.10
SDL2_MIXER:  https://github.com/libsdl-org/SDL_mixer/releases/tag/release-2.8.1
SDL2_IMAGE:  https://github.com/libsdl-org/SDL_image/releases/tag/release-2.8.8
```

HOW TO COMPILE IN WINDOW
```txt
WINDOWS (MinGW/GCC)
-------------------
1. Download SDL2, SDL2_image, SDL2_mixer development libraries (MinGW version)
2. Extract to a folder (e.g., C:\SDL2)
3. Set paths and compile: (check the correct path of your own machine)

SET SDL2_PATH=C:\SDL2\SDL2-2.x.x\x86_64-w64-mingw32
SET SDL2_IMAGE_PATH=C:\SDL2\SDL2_image-2.x.x\x86_64-w64-mingw32
SET SDL2_MIXER_PATH=C:\SDL2\SDL2_mixer-2.x.x\x86_64-w64-mingw32

gcc main.c -o main.exe ^
  -I.\ ^
  -I%SDL2_PATH%\include\SDL2 ^
  -I%SDL2_IMAGE_PATH%\include\SDL2 ^
  -I%SDL2_MIXER_PATH%\include\SDL2 ^
  -L%SDL2_PATH%\lib ^
  -L%SDL2_IMAGE_PATH%\lib ^
  -L%SDL2_MIXER_PATH%\lib ^
  -lmingw32 -lSDL2main -lSDL2 -lSDL2_image -lSDL2_mixer -lm

4. Copy DLLs to executable folder:
   - SDL2.dll
   - SDL2_image.dll
   - SDL2_mixer.dll
   - libpng16-16.dll (from SDL2_image)
   - zlib1.dll (from SDL2_image)

```


Teacher req:
```txt
+ Bản đồ và di chuyển: (3đ)
 - Có bản đồ lớn hơn screen ít nhất 4 lần: +1đ
 - Di chuyển trên bản đồ theo 1 trục (x hoặc y): +2đ
+ Tương tác với các đối tượng khác trong game: (3đ)
 - Giết địch: +1đ (ví dụ nhảy lên trên đầu)
 - Đập bể các “thùng” chứa item để “nâng cấp” player: +1đ
 - Lấy tiền: +1đ
+ Các đối tượng đặc biệt: (2đ)
 - Boss: +1đ
 - item “ngôi sao”, …: +1đ
+ Âm thanh và menu: (2đ)
 - Có nhạc nền và hiệu ứng (khi tương tác với các đối tượng): +1đ
 - Thiết kế menu có các tính năng cơ bản (Newgame, Option, About, Exit):+1đ
Phần thưởng:
 + Có nhiều cách tiêu diệt kẻ địch
 + Có những “chỗ khó” cần di chuyển các vật thể để có thể đi tiếp. VD: dời cây cầu để băng qua
sông, sắp các ô số đúng quy luật để mở cửa…
```

SPEC:
```txt
================================================================================
VERTICAL CLIMBER — MASTER SPEC  (v4)
================================================================================
Last updated : after terrain revamp, teleporter, location indicator, god mode
Status       : in active development
================================================================================


--------------------------------------------------------------------------------
0. FILE STRUCTURE
--------------------------------------------------------------------------------
00_main.c           entry point, run loop (untouched)
10_data.h           all structs, constants, globals
20_init.h           init + terrain layout + reset_game
30_control.h        input polling, menu/pause/win navigation, god mode toggle
40_process.h        TPS-gated logic dispatcher
41_process_helper.h movement, terrain collision, enemies, camera, win check
50_render.h         render dispatcher
51_render_helper.h  all draw functions (terrain, player, enemies, HUD, menus)

external_lib/ (untouched throughout):
  sdl2_wrapper.h    window, SDL2 draw primitives
  io_logic.h        mouse + keyboard signal structs
  time_util.h       timer_data, get_time_sec, wait_sec
  physic.h          physic_base_data, update_base, check_entity_collision
  common_logic.h    box collision, angle math


--------------------------------------------------------------------------------
1. WORLD
--------------------------------------------------------------------------------
World size   : 2000 x 14000 px
Screen size  : 1200 x 900 px  (4:3)
Camera       : 2D lerp follow (x and y), factor 0.15, clamped to world bounds
               screen_coord = world_coord - camera_coord
Coordinate   : origin top-left, Y increases downward
Player start : x=1000, y=13800  (near bottom center)
Summit       : y ~300
1m           : 140 px  →  100m total climb


--------------------------------------------------------------------------------
2. CONSTANTS  (10_data.h)
--------------------------------------------------------------------------------
WORLD_W=2000        WORLD_H=14000
SCREEN_W=1200       SCREEN_H=900
GRAVITY=1800.0      (px/s²)
MOVE_SPEED=280.0    MOVE_ACCEL=2000.0   MOVE_FRICTION=0.80
JUMP_VY=-700.0      JUMP_BOOST_VY=-950.0
DASH_SPEED=700.0    DASH_FRAMES=6
EDGE_GRAB_SLIDE=0.5 INVINCIBLE_FRAMES=40
TERRAIN_COUNT=240   COIN_COUNT=30   ITEM_COUNT=2   ENEMY_COUNT=7
STATE_MENU=0  STATE_PLAY=1  STATE_WIN=2  STATE_PAUSE=3
TERRAIN_SOLID=0  TERRAIN_BREAK=1
ENEMY_DASHER=0   ENEMY_BOSS=1
Max safe jump height ≈ 136px  (700²/2/1800)
Vertical gap rule: terrain tops spaced ≤ 110px apart


--------------------------------------------------------------------------------
3. PLAYER  (player_data in 10_data.h)
--------------------------------------------------------------------------------
Hitbox       : 28 x 36 px, centered on base.x/y
Physics      : update_base() from physic.h each tick
               gravity applied as base.ay = GRAVITY when not grabbing

Movement:
  A / D        horizontal, MOVE_ACCEL toward MOVE_SPEED, MOVE_FRICTION on release
  Space        jump (JUMP_VY or JUMP_BOOST_VY if boost active)
  Shift L/R    dash — direction = last_move_dir, DASH_FRAMES duration, resets on ground

Edge grab:
  Activates    when pressing A or D INTO a wall (key direction matches wall side)
               AND player not on ground
  While active vx=0, gravity off, vy *= EDGE_GRAB_SLIDE per tick (slow slide)
  Release      press opposite direction (drop) or Space (wall jump)
  Wall jump    push away from wall (vx = -wall_dir * 300) + JUMP_VY

No death:
  Falling between terrain just lands player on lower terrain.
  Bottom floor is solid, no kill trigger.
  Enemy hit causes invincibility + knockback only.

God mode (debug):
  G key        toggle on/off (STATE_PLAY only)
  WASD + Space free fly at 600px/s, all physics/collision skipped
  HUD          red GOD indicator top-right when active
  Reset        god_mode=0 on reset_game()

Fields:
  on_ground, on_wall(-1/0/1), edge_grab, grab_wall_dir
  jump_count, dash_ready, dashing, dash_dir
  invincible, god_mode
  respawn_x/y, score, last_move_dir, jump_boost_timer


--------------------------------------------------------------------------------
4. TERRAIN  (terrain_data[], 20_init.h)
--------------------------------------------------------------------------------
Each terrain is a physic_base_data rect (weight=-1, immovable).
Collision: loop all terrain[] in process_terrain_collision(),
           manual AABB overlap resolve (vertical priority over horizontal).
           physic.h check_entity_collision NOT used for terrain (one-body resolve).

Struct fields:
  base          physic_base_data (center x,y; col_w, col_h)
  type          TERRAIN_SOLID or TERRAIN_BREAK
  hp            -1=indestructible, 1=breaks on stomp
  broken        1=skip collision+render, countdown broken_timer to respawn
  broken_timer  300 frames (~15s) to respawn
  coin_inside   1=spawn coin at center when broken

Boundaries (always in terrain[]):
  Left wall   x=-20, w=20, full height
  Right wall  x=2000, w=20, full height
  Bottom floor x=0, y=14000, full width, h=60  (solid, no kill)
  Top cap     x=0, y=0, full width, h=20

Terrain variants (macros in 20_init.h):
  S(x,y,w,h)                     plain solid rect
  B(x,y,w,h, coin)               breakable rock, coin_inside flag
  stair(x,y,sw,sh, n,dx,dy)      n steps offset by dx,dy (staircase cliff)
  face(x,y,h)                    50px wide tall slab (grab climbing surface)
  hang(x,y,w)                    18px tall wide slab (overhang/hanging rock)
  boulder(x,y)                   3 irregular rects clustered (impassable mass)
  ceiling_chunk(x,y,w,h)         wide slab from above (cave ceiling)
  chimney(x,y,gap,h)             two face() walls, gap apart (bounce climb)

Color by altitude:
  y < 3000   snow white  0xDCDCECFF  top edge 0xFFFFFFFF
  y < 6000   gray rock   0x8A8A9AFF  top edge 0xBBBBCCFF
  y >= 6000  dark slate  0x5A5A6AFF  top edge 0x777788FF
  BREAK rock             0x9B6B4BFF  crack lines drawn

Layout: 20 vertical bands (~700px each), each band contains:
  - Two chimney or face walls on far left/right edges
  - Staircase climbs from lower ledge up
  - Ceiling chunks pressing down in center
  - Boulder clusters in center void
  - Hanging slabs mid-climb
  - Break rocks (B) scattered in each band
  - Wide merge platforms at summit approach (y 1400-2100)


--------------------------------------------------------------------------------
5. CAMERA  (camera_data in 10_data.h)
--------------------------------------------------------------------------------
struct camera_data { double x; double y; }
Update each tick:
  target_x = player.base.x - SCREEN_W/2
  target_y = player.base.y - SCREEN_H/2
  camera.x += (target_x - camera.x) * 0.15
  camera.y += (target_y - camera.y) * 0.15
  clamp: x in [0, WORLD_W-SCREEN_W], y in [0, WORLD_H-SCREEN_H]


--------------------------------------------------------------------------------
6. COINS  (coin_data[], 10_data.h)
--------------------------------------------------------------------------------
COIN_COUNT=30. Placed on edges of terrain, hard spots.
Collision: 24x24 box centered on coin vs player box.
On collect: collected=1, player.score++. No respawn.
Visual: gold 16x16 outer, lighter 9x9 inner.


--------------------------------------------------------------------------------
7. JUMP BOOST ITEM  (item_data[], 10_data.h)
--------------------------------------------------------------------------------
ITEM_COUNT=2. Placed at mid-mountain (y~5940, y~3140).
On collect: player.jump_boost_timer=200 (10s at 20TPS),
            item.active=0, item.respawn_timer=400 (20s).
Effect: JUMP_VY replaced by JUMP_BOOST_VY while timer>0.
Visual: layered star (yellow/orange/white rects).
HUD: "BOOST" indicator top-center when active.


--------------------------------------------------------------------------------
8. ENEMIES  (enemy_data[], 10_data.h)
--------------------------------------------------------------------------------
ENEMY_COUNT=7  (6 ENEMY_DASHER + 1 ENEMY_BOSS)

ENEMY_DASHER behavior:
  - Patrol left/right on patrol_y (snapped each tick, no gravity)
  - Every ~80-100 frames dash toward player (12 frames, dash_vx)
  - Stomp: player vy>0, player_bottom <= enemy_top+14 → stun 60 frames, bounce player
  - Hit: player.invincible==0 → push + knockback, set invincible=40

ENEMY_BOSS (x1, y~1415, summit approach):
  - Same as dasher but 64x64, hp=5, stun 80 frames per hit
  - At hp=0: permanent defeat (stun_timer=99999)
  - Dash every ~45 frames, dash_vx=520
  - Wider patrol (x 380-1620)
  - Stronger push on hit (vx=500, vy=-450)
  - HP bar drawn above boss

Placement: pairs of dashers at y~10420, y~7660, y~4900 (left+right edges).


--------------------------------------------------------------------------------
9. WIN CONDITION  (teleporter)
--------------------------------------------------------------------------------
No auto-win on Y position — player explores summit freely.

Teleporter cage: world center (1000, 260), size 80x80.
  Drawn as: cyan glow aura, cage bars (rects), portal interior glow, "ENTER" label.
  Collision: check_two_box_2d_hit_centralized vs player → STATE_WIN.

Summit area (y~300, win platform x 560-1440):
  Tree (left, x~680): trunk + layered foliage + snow caps
  Coffee table (right, x~1300): tabletop + legs + mug
  Sign board (x~870): "SUMMIT" text
  Boulder decor at platform edges
  Teleporter cage (x~1000): glowing cyan box, interaction point

Win screen overlay:
  "SUMMIT REACHED" title
  "COINS N" display
  [PLAY AGAIN] button (x=500,y=450,w=200,h=50)
  Space also replays
  On replay: reset_game() → STATE_PLAY


--------------------------------------------------------------------------------
10. MENU  (STATE_MENU)
--------------------------------------------------------------------------------
Sub-states: 0=main, 1=options, 2=about
Main: NEW GAME / OPTIONS / ABOUT / EXIT  (mouse click or Enter)
Options: key bindings text
About: credits text
Escape: returns to main sub-state from options/about
Visual: dark bg, mountain silhouette, snow dot particles, title text


--------------------------------------------------------------------------------
11. HUD  (draw_hud in 51_render_helper.h)
--------------------------------------------------------------------------------
Top-left      COINS N  (gold text, dark bg box)
Top-center    Xm  +  progress bar (green fill)
              BOOST indicator below bar when active
Bottom-center Location line: "[NS][EW]  X:####  Y:####"
              N/S from player y (<3000=N, >11000=S)
              E/W from player x (<600=W, >1400=E)
Top-right     GOD  (red box, when god_mode=1)
              GRAB (teal box, when edge_grab=1)


--------------------------------------------------------------------------------
12. TIMING
--------------------------------------------------------------------------------
TPS=20  (1/20s logic tick via timer_data)
Render on render_flag=1 per tick (set at end of each logic tick).
wait_sec() called when tick not yet due.


--------------------------------------------------------------------------------
13. SOUND  (deferred)
--------------------------------------------------------------------------------
SDL_mixer already linked in sdl2_wrapper.h.
Stub calls: play_sound(SOUND_JUMP), play_sound(SOUND_DASH), etc.
Files needed: jump.wav, dash.wav, coin.wav, stomp.wav, hurt.wav, bgm.ogg
TODO: wire up once assets exist.


================================================================================
TODO LIST
================================================================================

--- TERRAIN ---
[ ] Design two distinct named routes:
      LEFT PATH  — parkour-heavy, tight gaps, chimney climbs, minimal enemies
      RIGHT PATH — combat-heavy, wider ledges, 4 enemies + boss
    Currently terrain is evenly filled with no deliberate routing.
    Plan: place deliberate void gaps between left/right zones per band,
          force path choice at base, reunite at summit approach (~y 1750).

[ ] False routes (4 planned, not yet placed):
      F1 (y~12500): ceiling trap, coin lure at entrance
      F2 (y~9500):  pit trap — visual gap, no floor below
      F3 (y~5500):  sealed cave right side, ceiling seals upward exit
      F4 (y~3000):  bridge to nowhere, ceiling slab blocks upward

[ ] Polish terrain density per zone:
      Base (y 13300-14000): good, leave as is
      Zones 1-4: chimney gaps may still be too large, test in-game
      Summit approach (y 700-2100): currently wide open, add ceiling chunks
        and boulders to maintain cave feel even at the top

[ ] Add terrain visual polish:
      Rocky texture approximation: add 1-2 darker rect strips on large slabs
      Icicle decor on ceiling_chunk tops in snow zone (y<3000)
      Crack detail lines on B() break rocks (currently just X lines)

--- ENEMIES ---
[ ] Tune dasher frequency per zone:
      Lower zones (y>10000): slower dash (timer 100-120)
      Upper zones (y<5000):  faster dash (timer 50-70)
    Currently all dashers have same speed.

[ ] Boss visual polish:
      Add attack animation (color flash on dash)
      Add defeat animation (stay grey, eyes X, don't respawn)
      Currently defeat is just stun_timer=99999 with no visual feedback

--- PLAYER ---
[ ] Tune jump feel:
      JUMP_VY=-700 may still feel low for chimney climbs, test in-game
      Consider coyote time (2-3 frame grace after walking off ledge)
      Consider variable jump height (hold Space = full jump, tap = short)

[ ] Dash polish:
      Currently dash cancels gravity (ay=0 during dash)
      Consider: allow slight downward arc during dash for better feel
      Dash recharge: currently resets on ground only
      Consider: also recharge on wall grab (more Celeste-like)

--- WIN / SUMMIT ---
[ ] Teleporter cage animation:
      Currently static rects. Add pulsing glow (vary rect alpha over time).
      Need a frame counter or use tps_timer.dt_sec_since_start for time.

[ ] Win screen:
      Currently plain overlay. Add simple score breakdown (coins/total).
      Add best-time tracking (use get_time_sec() from run start).

--- HUD ---
[ ] Minimap (optional):
      Small 100x200px minimap top-right showing world outline + player dot.
      Useful for finding false routes.

[ ] Enemy proximity warning:
      Small indicator when enemy is off-screen but nearby.

--- SOUND ---
[ ] Wire SDL_mixer:
      load_sound() calls in init()
      play_sound() calls at: jump, dash, coin collect, stomp, hurt, boss defeat
      BGM loop from menu start

--- MENU ---
[ ] Add high score / best time to menu screen
[ ] Options screen: add volume slider placeholder, fullscreen toggle placeholder

--- CODE QUALITY ---
[ ] Move terrain band definitions to a separate 20_terrain.h
    to keep 20_init.h under 300 lines
[ ] Add #define DEBUG_SHOW_HITBOXES toggle:
    when on, draw player + enemy + terrain hitbox outlines in render
[ ] Review terrain collision loop performance:
    currently O(n) all 240 terrain each tick
    spatial hash or simple Y-band cull would help if TPS drops

================================================================================
END OF MASTER SPEC v4
================================================================================
```
