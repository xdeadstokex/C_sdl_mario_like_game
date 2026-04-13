# C_sdl_mario_like_game

DES:

```txt
Vertical Climber, 2D platformer, reach the top
that simple

Controls:
  A/D         move left/right (grab walls by press A D against wall)
  Space       jump, wall jump (while edge_grab)
  Shift L/R   dash (direction = last_move_dir), resets on ground
  G           toggle god mode (free fly WASD+Space, skips all physics)
  K           hot reload world file (preserves player)
  I / O       zoom in / out (+/-10 px_per_m), snaps camera instantly
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
  -lmingw32 -lSDL2main -lSDL2 -lSDL2_image -lSDL2_mixer -lm -lws2_32

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
VERTICAL CLIMBER — MASTER SPEC (v5)
================================================================================
Last updated: decor system, unit system (meters), world loader, game_config
================================================================================


--------------------------------------------------------------------------------
0. FILE STRUCTURE
--------------------------------------------------------------------------------
00_main.c               entry, run loop (untouched)
10_data.h               fixed constants, all global instances
20_init.h               init, reset_game(), reload_world(), snap_camera()
30_control.h            input: WASD/space/shift, G=god, K=reload, I/O=zoom
40_process.h            TPS-gated logic dispatcher (20 TPS)
41_process_helper.h     all game logic (meters/m/s/m/s²)
50_render.h             render dispatcher
51_render_helper.h      all draw (SX/SY/SP macros, meter->pixel at boundary)

external_lib/           reusable, no app-header dependencies
  physic.h              physic_base_data, update_base, check_entity_collision
  io_logic.h            mouse/keyboard signal structs
  time_util.h           timer_data, get_time_sec, wait_sec
  sdl2_wrapper.h        SDL2 window, draw primitives
  common_logic.h        box collision, angle math
  game_obj.h            all game structs + game_config + m2sx/m2sy/m2sp
  world_loader.h        file loader, Y-flip, per-tag helpers, wl_ctx


--------------------------------------------------------------------------------
1. UNIT SYSTEM
--------------------------------------------------------------------------------
All game logic (position, velocity, acceleration, size) is in METERS.
Render converts to pixels only at draw time:

  SX(world_x_m)  = (int)(world_x_m * cfg.px_per_m - camera.x_px)
  SY(world_y_m)  = (int)(world_y_m * cfg.px_per_m - camera.y_px)
  SP(size_m)     = (int)(size_m    * cfg.px_per_m)

Camera is in pixels (viewport rectangle).
physic.h is unit-agnostic (pure math, works in any unit).


--------------------------------------------------------------------------------
2. WORLD FILE  (resource/world.txt)
--------------------------------------------------------------------------------
Tags:
  V  key  value                              game config value
  T  x  y  w  h  type(solid|break)  coin    terrain rect
  E  x  y  type(dasher|boss)  pmin  pmax    enemy
  C  x  y                                   coin
  I  x  y                                   jump boost item
  P  x  y                                   player spawn (first line used)
  D  x  y  w  h  color(0xRRGGBBAA) [label]  decor rect
             label="teleporter" = animated cage + win trigger

Coordinate convention in file:
  x = meters from left edge
  y = meters from GROUND (y=0 = ground, increases UPWARD like real life)
  Loader Y-flips to physic space: physic_y = world_h - file_y (- height for rects)

Comments: lines starting with #
Blank lines: skipped
On parse error: fprintf(stderr) + exit(1)

Hot reload: K key reloads file, preserves player position + zoom level (px_per_m).
New game / play again: full reload including player spawn from P tag.

V keys (all optional, defaults apply):
  px_per_m  world_w  world_h
  gravity  move_speed  move_accel  move_friction
  jump_vy  jump_boost_vy  dash_speed  dash_frames
  edge_grab_slide  invincible_frames  player_w  player_h
  max_vx  max_vy


--------------------------------------------------------------------------------
3. GAME CONFIG  (struct game_config cfg  in 10_data.h)
--------------------------------------------------------------------------------
Runtime struct, loaded from V tags. Defaults:
  px_per_m=100      world_w=20        world_h=140
  gravity=18        move_speed=2.8    move_accel=20
  move_friction=0.8 jump_vy=-7        jump_boost_vy=-9.5
  dash_speed=7      dash_frames=6     edge_grab_slide=0.5
  invincible_frames=40  player_w=0.28 player_h=0.36
  max_vx=10         max_vy=20         (velocity caps, prevents tunnelling)
  screen_w/h        set from window.w/h after init_window (not from file)


--------------------------------------------------------------------------------
4. FIXED CONSTANTS  (10_data.h, structural only)
--------------------------------------------------------------------------------
TERRAIN_COUNT=240  COIN_COUNT=30  ITEM_COUNT=2
ENEMY_COUNT=7      DECOR_COUNT=64
STATE_MENU=0  STATE_PLAY=1  STATE_WIN=2  STATE_PAUSE=3
TERRAIN_SOLID=0  TERRAIN_BREAK=1
ENEMY_DASHER=0   ENEMY_BOSS=1


--------------------------------------------------------------------------------
5. PLAYER  (struct player_data)
--------------------------------------------------------------------------------
physic_base_data base   center in meters, gravity applied as base.ay
on_ground, on_wall(-1/0/1), edge_grab, grab_wall_dir
jump_count, dash_ready, dashing, dash_dir
invincible (frames), god_mode
respawn_x/y, score, last_move_dir, jump_boost_timer

Controls:
  A/D         move left/right
  Space       jump, wall jump (while edge_grab)
  Shift L/R   dash (direction = last_move_dir), resets on ground
  G           toggle god mode (free fly WASD+Space, skips all physics)
  K           hot reload world file (preserves player)
  I / O       zoom in / out (+/-10 px_per_m), snaps camera instantly
  F           fire

Edge grab:
  Activates   pressing A or D INTO a wall while airborne
  Hold        same key holds grab; vy *= edge_grab_slide each tick; gravity off
  Release     press opposite key (drop) or Space (wall jump)
  Wall jump   vx = -wall_dir * 3.0 m/s + full jump_vy

Update: Hp now appear (init maybe 5 hp)
Tobecontinued

Velocity caps (applied after update_base each tick):
  vx clamped to [-max_vx, +max_vx]
  vy clamped to [-max_vy, +max_vy]
  Prevents tunnelling through thin terrain at high speed.


--------------------------------------------------------------------------------
6. TERRAIN  (struct terrain_data[]) **HEIGHT must > 0.5
--------------------------------------------------------------------------------
physic_base_data base   center x,y in meters; col_w/h in meters
type: TERRAIN_SOLID (-1 hp) or TERRAIN_BREAK (hp=1)
broken: 1 = skip collision+render; broken_timer=300 ticks to respawn
coin_inside: 1 = spawn coin when broken (handled externally if needed)

Collision: manual AABB per terrain rect vs player each tick.
Vertical overlap resolved first (landing/ceiling), then horizontal (walls).
Break blocks stomp-broken when player vy>0 landing on top; bounce player vy=-2.5.


--------------------------------------------------------------------------------
7. CAMERA  (struct camera_data)
--------------------------------------------------------------------------------
x_px, y_px: top-left of viewport in pixels
Update each tick: lerp toward player center, factor 0.15
No clamping — world boundary walls (solid terrain) keep player in bounds.
Zoom snap (I/O keys): camera.x_px/y_px recalculated immediately, no lerp delay.


--------------------------------------------------------------------------------
8. ENEMIES  (struct enemy_data[])
--------------------------------------------------------------------------------
ENEMY_DASHER  32x32 cm, hp=1, stun 60 ticks, dash_vx=3.5 m/s
ENEMY_BOSS    64x64 cm, hp=5, stun 80 ticks/permanent at hp=0, dash_vx=5.2 m/s

Behavior:
  Patrol left/right on patrol_y at 1 m/s
  Every ~80-100 ticks dash toward player (12 ticks)
  Stomp (player vy>0, bottom <= enemy top+0.14m): stun, bounce player
  Hit player: invincible_frames + knockback push

patrol_x_min/max from file E tag.


--------------------------------------------------------------------------------
9. COINS  (struct coin_data[])
--------------------------------------------------------------------------------
Collision: 0.24x0.24m box. On collect: score++, collected=1. No respawn.


--------------------------------------------------------------------------------
10. ITEMS  (struct item_data[])
--------------------------------------------------------------------------------
Jump boost: jump_boost_timer=200 ticks on collect.
Active=0 after collect; (respawn after 400 ticks only for the ones outside the chests)
Effect: JUMP_BOOST_VY used instead of JUMP_VY while timer>0.

Speed boost: speed_boost_timer=200 ticks on collect.
Active=0 after collect; (respawn after 400 ticks only for the ones outside the chests)
Effect: SPEED_BOOST (*1.5)

Fireball: 5 Fireball, press F to fire, deal 1 dame



--------------------------------------------------------------------------------
11. DECOR  (struct decor_data[])
--------------------------------------------------------------------------------
x,y,w,h in meters (physic space, Y-flipped on load from file).
color: 0xRRGGBBAA
label[32]: drawn centered on rect if non-empty
is_teleporter=1: drawn as animated cyan cage; collision triggers STATE_WIN

Win condition: player overlaps any decor with is_teleporter=1.
No hardcoded win Y position.

File tag: D x y w h color [label or "teleporter"]
  plain rect:   D 8.7 1.28 1.6 0.32 0xF5DEB3FF SUMMIT
  teleporter:   D 9.6 2.2  0.8 0.8  0x00FFFF44 teleporter


--------------------------------------------------------------------------------
12. HUD  (screen-space, not world-space)
--------------------------------------------------------------------------------
Top-left    COINS N
Top-center  compass (NS/EW from world position) + X/Y in meters + progress bar
            height = cfg.world_h - player.base.y  (0=ground, world_h=summit)
            BOOST indicator below bar when active
Top-right   GOD (red) when god_mode; GRAB (teal) when edge_grab


--------------------------------------------------------------------------------
13. MENU  (STATE_MENU)
--------------------------------------------------------------------------------
Sub-states: 0=main, 1=options, 2=about
Buttons: NEW GAME / OPTIONS / ABOUT / EXIT
Enter = new game. Mouse click on buttons.
Escape from sub-state = back to main.


--------------------------------------------------------------------------------
14. TIMING
--------------------------------------------------------------------------------
TPS=20 via timer_data tps_timer.
render_flag=1 set each logic tick; render only when flag set.
wait_sec() called when tick not yet due.


--------------------------------------------------------------------------------
15. SOUND  (deferred)
--------------------------------------------------------------------------------
SDL_mixer linked in sdl2_wrapper.h.
Calls needed: jump, dash, coin, stomp, hurt, boss_defeat, bgm loop.
Wire up once audio assets exist.


--------------------------------------------------------------------------------
TODO
--------------------------------------------------------------------------------
[ ] Design full world.txt map:
      Left path  — parkour, tight gaps, chimneys, few enemies
      Right path — combat, wider ledges, 4 dasher + boss
      False routes (4): ceiling traps + pit traps
      All decor (tree, table, sign, teleporter) placed at summit

[ ] Tune physics feel in-game via V tags, no recompile needed

[ ] Sound: SDL_mixer wiring + asset files

[ ] Boss defeat visual: grey out permanently, X eyes

[ ] Teleporter pulse animation: vary glow alpha over time using tps timer

[ ] Win screen: show coin count, add best-time tracking

[ ] Debug hitbox toggle: #define DEBUG_HITBOX, draw outlines in render

[ ] Terrain collision perf: Y-band cull if 240 rects causes TPS drop

[ ] Menu: high score / best time display

================================================================================
END OF SPEC v5
================================================================================
```
