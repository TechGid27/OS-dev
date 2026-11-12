; game.asm - DUKE: FOLKLORE SHOOTER (32-bit protected mode)
; entry point, main loop, game state machine

BITS 32
ORG  0x10000

%define FB       0x40000            ; offscreen framebuffer (64000 bytes)
%define VIDMEM   0xA0000            ; VGA mode 13h display memory
%define SCREEN_W 320
%define SCREEN_H 200

; ---- palette indices ----
%define C_TRANS   0                 ; transparent marker (never drawn)
%define C_BLACK   1
%define C_WHITE   2
%define C_SKIN    3
%define C_SKIN2   4
%define C_BLUE    5
%define C_RED     6
%define C_GREEN   7
%define C_YELLOW  8
%define C_PURPLE  9
%define C_GRAYD  10
%define C_GRAYL  11
%define C_BROWN  12
%define C_CYAN   13
%define C_ORANGE 14
%define C_PINK   15
; ramps
%define C_SKY0   16                 ; 16..19  sky
%define C_GND0   20                 ; 20..23  ground
%define C_BRK0   24                 ; 24..27  brick
%define C_HPBG   28                 ; 28..31  hp bar bg (red ramp)
%define C_HPFG   32                 ; 32..35  hp bar fg (green ramp)
%define C_GOLD0  36                 ; 36..39  ammo/key
%define C_ASW0   40                 ; 40..43  aswang
%define C_KAP0   44                 ; 44..47  kapre
%define C_DOOR0  48                 ; 48..51  door
%define C_UI0    52                 ; 52..55  hud/steel
%define C_DARK0  56                 ; 56..59  dark
%define C_ACC0   60                 ; 60..63  accent

; ---- game states ----
%define ST_TITLE    0
%define ST_PLAY     1
%define ST_DEAD     2
%define ST_GAMEOVER 3
%define ST_CLEAR    4

; ---- keyboard scancodes ----
%define KEY_LEFT   0x4B
%define KEY_RIGHT  0x4D
%define KEY_UP     0x48
%define KEY_DOWN   0x50
%define KEY_SPACE  0x39
%define KEY_CTRL   0x1D
%define KEY_ENTER  0x1C
%define KEY_Z      0x2C
%define KEY_X      0x2D
%define KEY_R      0x13
%define KEY_ESC    0x01

; ---- tile ids ----
%define T_EMPTY   0
%define T_GROUND  1
%define T_PLAT    2
%define T_DOOR    3
%define T_EXIT    4
%define T_GROUND_TOP 5

; ---- spawn types ----
%define SPAWN_PLAYER 0
%define SPAWN_ASWANG 1
%define SPAWN_KAPRE  2
%define SPAWN_HEALTH 3
%define SPAWN_AMMO   4
%define SPAWN_KEY    5
%define SPAWN_TIKBALANG   6
%define SPAWN_MANANANGGAL 7
%define SPAWN_BOSS        8

; ---- physics (8.8 fixed point) ----
%define GRAVITY      80             ; 0.31 px/t^2
%define MAX_FALL     1792           ; 7.0 px/t
%define RUN_SPEED    640            ; 2.5 px/t
%define JUMP_V      -1536           ; -6.0 px/t
%define BULLET_SPEED 1536           ; 6.0 px/t

; =====================================================================
; entry
; =====================================================================
game_main:
    cli
    mov esp, 0x9FF00                ; stack top moved below 0xA0000 (OS data at 0x80000-0x9EFFF)

    call video_init                 ; palette + clear framebuffer
    call charset_init               ; char -> color map
    call idt_build                  ; IDT gates + lidt
    call timer_init                 ; PIT 60 Hz
    call int_init                   ; remap PIC, unmask IRQ0/1, sti

    ; camera at player spawn
    movzx eax, word [level_player_spawn]
    add  eax, 8
    movzx ebx, word [level_player_spawn + 2]
    add  ebx, 8
    call camera_follow

    mov  dword [game_state], ST_TITLE
    mov  dword [last_tick], 0
    mov  byte [ui_en_prev], 0
    mov  byte [ui_r_prev], 0
.main:
    mov  eax, [tick_count]
    cmp  eax, [last_tick]
    je   .wait
    mov  [last_tick], eax

    call game_update
    call game_draw
    call video_flip
.wait:
    hlt
    jmp  .main

last_tick: dd 0

; =====================================================================
; update dispatch
; =====================================================================
game_update:
    ; -- ESC: back to the OS file browser (kernel entry at 0x60000) --
    cmp  byte [key_state + KEY_ESC], 1
    jne  .noesc
    jmp  0x60000
.noesc:
    ; -- ENTER edge --
    mov  al, [key_state + KEY_ENTER]
    mov  [ui_en_now], al
    mov  byte [ui_en_edge], 0
    cmp  al, 1
    jne  .ne
    cmp  byte [ui_en_prev], 1
    je   .ne
    mov  byte [ui_en_edge], 1
.ne:
    ; -- R edge --
    mov  al, [key_state + KEY_R]
    mov  [ui_r_now], al
    mov  byte [ui_r_edge], 0
    cmp  al, 1
    jne  .nr
    cmp  byte [ui_r_prev], 1
    je   .nr
    mov  byte [ui_r_edge], 1
.nr:
    ; -- dispatch --
    mov  eax, [game_state]
    cmp  eax, ST_TITLE
    je   .title
    cmp  eax, ST_PLAY
    je   .play
    cmp  eax, ST_DEAD
    je   .dead
    cmp  eax, ST_GAMEOVER
    je   .ovr
    cmp  eax, ST_CLEAR
    je   .clr
    jmp  .done
.title:
    cmp  byte [ui_en_edge], 1
    jne  .done
    call start_game
    jmp  .done
.play:
    call play_update
    jmp  .done
.dead:
    call dead_update
    jmp  .done
.ovr:
    cmp  byte [ui_r_edge], 1
    jne  .done
    mov  dword [game_state], ST_TITLE
    jmp  .done
.clr:
    mov  eax, [cur_stage]
    inc  eax
    cmp  eax, [stage_total]
    jae  .final
    cmp  byte [ui_en_edge], 1           ; more stages: ENTER goes on
    jne  .done
    call next_stage
    jmp  .done
.final:
    cmp  byte [ui_r_edge], 1
    jne  .done
    mov  dword [game_state], ST_TITLE
    jmp  .done
.done:
    mov  al, [ui_en_now]
    mov  [ui_en_prev], al
    mov  al, [ui_r_now]
    mov  [ui_r_prev], al
    ret

; =====================================================================
start_game:
    mov  dword [cur_stage], 0
    xor  eax, eax
    call stage_load
    call player_restart
    call enemies_init
    call pickups_init
    movzx eax, word [level_player_spawn]
    add  eax, 8
    movzx ebx, word [level_player_spawn + 2]
    add  ebx, 8
    call camera_follow
    mov  dword [game_state], ST_PLAY
    ret

; =====================================================================
; stage progression
; =====================================================================
; stage_load: eax = 0-based stage index. Copies that stage's map, spawn
; list and player spawn into the RAM buffers used by the rest of the game.
stage_load:
    imul ebx, eax, 12
    mov  esi, [stage_table + ebx]       ; map tiles
    mov  edi, map_data
    mov  ecx, MAP_W * MAP_H
    rep  movsb

    mov  esi, [stage_table + ebx + 4]   ; spawn list: count, then entries
    movzx ecx, byte [esi]
    mov  [level_spawn_count], cl
    inc  esi
    imul ecx, ecx, 5
    mov  edi, level_spawns
    rep  movsb

    mov  esi, [stage_table + ebx + 8]   ; player spawn: dw x, dw y
    mov  edi, level_player_spawn
    mov  ecx, 2
    rep  movsw
    ret

; next_stage: go on to the following stage. Lives and ammo carry over,
; hp is refilled, the key is taken back.
next_stage:
    inc  dword [cur_stage]
    mov  eax, [cur_stage]
    call stage_load
    mov  dword [pl_hp], 100
    mov  byte [pl_has_key], 0
    call player_init
    call enemies_init
    call pickups_init
    movzx eax, word [level_player_spawn]
    add  eax, 8
    movzx ebx, word [level_player_spawn + 2]
    add  ebx, 8
    call camera_follow
    mov  dword [game_state], ST_PLAY
    ret

; =====================================================================
play_update:
    call player_update
    call enemies_update
    call bullets_update
    call eb_update
    call pickups_update
    call door_exit_check

    ; camera follows player center
    mov  eax, [pl_x]
    sar  eax, 8
    add  eax, 8
    mov  ebx, [pl_y]
    sar  ebx, 8
    add  ebx, 8
    call camera_follow

    ; fell into a pit? (bottom of world only reachable there)
    mov  eax, [pl_y]
    cmp  eax, 320 << 8
    jb   .hpcheck
    mov  dword [pl_hp], 0
    call sfx_hurt
.hpcheck:
    ; death?
    cmp  dword [pl_hp], 0
    jg   .alive
    mov  dword [game_state], ST_DEAD
    mov  dword [dead_timer], 90
.alive:
    ret

; =====================================================================
dead_update:
    mov  eax, [dead_timer]
    dec  dword [dead_timer]
    and  al, 1
    mov  [pl_invuln], al            ; blink corpse
    cmp  dword [dead_timer], 0
    jg   .wait
    dec  byte [pl_lives]
    je   .gameover
    ; respawn, keep lives/ammo, key is lost on death
    call enemies_init
    call pickups_init
    mov  dword [pl_hp], 100
    mov  byte [pl_has_key], 0
    call player_init
    movzx eax, word [level_player_spawn]
    add  eax, 8
    movzx ebx, word [level_player_spawn + 2]
    add  ebx, 8
    call camera_follow
    mov  dword [game_state], ST_PLAY
    ret
.gameover:
    mov  dword [game_state], ST_GAMEOVER
    call sfx_gameover
    ret
.wait:
    ret

; =====================================================================
; door: solid while locked - opens when player faces it with key
; exit: passable tile - touching it clears the level
door_exit_check:
    ; --- front tile at torso height ---
    mov  eax, [pl_x]
    sar  eax, 8
    cmp  byte [pl_dir], 0
    jne  .fl
    add  eax, 16
    jmp  .fc
.fl:
    sub  eax, 1
.fc:
    sar  eax, 4
    mov  ecx, eax                   ; tx
    mov  eax, [pl_y]
    sar  eax, 8
    add  eax, 8
    sar  eax, 4
    mov  ebx, eax                   ; ty
    mov  eax, ecx
    call tile_at
    cmp  eax, T_DOOR
    jne  .exit
    cmp  byte [pl_has_key], 1
    jne  .done
    ; open: clear door tile
    mov  eax, [pl_x]
    sar  eax, 8
    cmp  byte [pl_dir], 0
    jne  .fl2
    add  eax, 16
    jmp  .fc2
.fl2:
    sub  eax, 1
.fc2:
    sar  eax, 4
    mov  ecx, eax
    mov  eax, [pl_y]
    sar  eax, 8
    add  eax, 8
    sar  eax, 4
    mov  ebx, eax
    mov  eax, ecx
    imul ebx, ebx, MAP_W
    add  eax, ebx
    mov  byte [map_data + eax], T_EMPTY
    call sfx_door
    jmp  .done
.exit:
    ; --- center tile ---
    mov  eax, [pl_x]
    sar  eax, 8
    add  eax, 8
    sar  eax, 4
    mov  ecx, eax
    mov  eax, [pl_y]
    sar  eax, 8
    add  eax, 8
    sar  eax, 4
    mov  ebx, eax
    mov  eax, ecx
    call tile_at
    cmp  eax, T_EXIT
    jne  .done
    cmp  dword [game_state], ST_PLAY
    jne  .done
    call enemies_alive                  ; exit only counts when all are dead
    test eax, eax
    jnz  .done
    mov  dword [game_state], ST_CLEAR
    call sfx_clear
.done:
    ret

; =====================================================================
; draw dispatch
; =====================================================================
game_draw:
    mov  eax, [game_state]
    cmp  eax, ST_TITLE
    je   draw_title
    cmp  eax, ST_PLAY
    je   draw_play
    cmp  eax, ST_DEAD
    je   draw_play
    cmp  eax, ST_GAMEOVER
    je   draw_gameover
    cmp  eax, ST_CLEAR
    je   draw_clear
    ret

draw_play:
    call draw_backdrop
    call draw_map
    call pickups_draw
    call enemies_draw
    call bullets_draw
    call player_draw
    call hud_draw
    ret

; draw_panel: eax=x ebx=y ecx=w edx=h esi=body colour
; dark UI plate with black border (matches the HUD style)
draw_panel:
    mov  [dp_x], eax
    mov  [dp_y], ebx
    mov  [dp_w], ecx
    mov  [dp_h], edx
    mov  [dp_col], esi
    dec  eax
    dec  ebx
    add  ecx, 2
    add  edx, 2
    mov  esi, C_BLACK
    call fill_rect
    mov  eax, [dp_x]
    mov  ebx, [dp_y]
    mov  ecx, [dp_w]
    mov  edx, [dp_h]
    mov  esi, [dp_col]
    call fill_rect
    ret

draw_title:
    call draw_backdrop
    ; logo plate
    mov  eax, 16
    mov  ebx, 16
    mov  ecx, 288
    mov  edx, 64
    mov  esi, C_DARK0 + 1
    call draw_panel
    mov  esi, str_title
    mov  ebx, 128
    mov  ecx, 28
    mov  edx, C_ORANGE
    call draw_text_2x
    mov  esi, str_sub
    mov  ebx, 32
    mov  ecx, 56
    mov  edx, C_WHITE
    call draw_text_2x
    ; prompt plate
    mov  eax, 92
    mov  ebx, 98
    mov  ecx, 136
    mov  edx, 30
    mov  esi, C_DARK0 + 1
    call draw_panel
    ; blink prompt
    test dword [tick_count], 16
    jz   .noprompt
    mov  esi, str_start
    mov  ebx, 116
    mov  ecx, 104
    mov  edx, C_YELLOW
    call draw_text
.noprompt:
    mov  esi, str_quitkey
    mov  ebx, 100
    mov  ecx, 116
    mov  edx, C_GRAYL
    call draw_text
    ; hints plate
    mov  eax, 20
    mov  ebx, 130
    mov  ecx, 280
    mov  edx, 38
    mov  esi, C_DARK0 + 1
    call draw_panel
    mov  esi, str_hint1
    mov  ebx, 28
    mov  ecx, 138
    mov  edx, C_GRAYL
    call draw_text
    mov  esi, str_hint2
    mov  ebx, 48
    mov  ecx, 152
    mov  edx, C_GRAYL
    call draw_text
    ; decorative sprites
    mov  eax, spr_player_idle
    xor  ebx, ebx
    mov  ebx, 140
    mov  ecx, 174
    xor  edx, edx
    mov  esi, 16
    mov  edi, 16
    call draw_ascii
    mov  eax, spr_kapre1
    mov  ebx, 172
    mov  ecx, 174
    xor  edx, edx
    mov  esi, 16
    mov  edi, 16
    call draw_ascii
    ret

draw_gameover:
    call draw_backdrop
    mov  eax, 48
    mov  ebx, 56
    mov  ecx, 224
    mov  edx, 80
    mov  esi, C_HPBG + 1               ; dark red plate
    call draw_panel
    mov  esi, str_over
    mov  ebx, 88
    mov  ecx, 70
    mov  edx, C_WHITE
    call draw_text_2x
    mov  esi, str_restart
    mov  ebx, 104
    mov  ecx, 110
    mov  edx, C_YELLOW
    call draw_text_2x
    ret

draw_clear:
    call draw_backdrop
    mov  eax, 48
    mov  ebx, 56
    mov  ecx, 224
    mov  edx, 80
    mov  esi, C_DARK0 + 1
    call draw_panel
    mov  eax, [cur_stage]
    inc  eax
    cmp  eax, [stage_total]
    jae  .final
    mov  esi, str_stage_clr
    mov  ebx, 64
    mov  ecx, 66
    mov  edx, C_YELLOW
    call draw_text_2x
    mov  esi, str_next
    mov  ebx, 56
    mov  ecx, 110
    mov  edx, C_WHITE
    call draw_text
    ret
.final:
    mov  esi, str_won
    mov  ebx, 72
    mov  ecx, 70
    mov  edx, C_YELLOW
    call draw_text_2x
    mov  esi, str_restart
    mov  ebx, 104
    mov  ecx, 110
    mov  edx, C_WHITE
    call draw_text_2x
    ret

; =====================================================================
section .data
game_state: dd 0
cur_stage:  dd 0                    ; 0-based index into stage_table
dead_timer: dd 0
dp_x: dd 0
dp_y: dd 0
dp_w: dd 0
dp_h: dd 0
dp_col: dd 0
ui_en_now:  db 0
ui_en_prev: db 0
ui_en_edge: db 0
ui_r_now:   db 0
ui_r_prev:  db 0
ui_r_edge:  db 0
str_title:   db "DUKE", 0
str_sub:     db "FOLKLORE SHOOTER", 0
str_start:   db "PRESS ENTER", 0
str_quitkey: db "ESC: QUIT TO OS", 0
str_hint1:   db "ARROWS MOVE  SPACE JUMP  CTRL FIRE", 0
str_hint2:   db "FIND THE KEY  REACH THE EXIT", 0
str_over:    db "GAME OVER", 0
str_won:     db "GAME CLEAR!", 0
str_stage_clr: db "STAGE CLEAR!", 0
str_next:    db "PRESS ENTER FOR NEXT STAGE", 0
str_restart: db "PRESS R", 0

%include "level.inc"
%include "art.inc"
%include "interrupts.inc"
%include "timer.inc"
%include "input.inc"
%include "sound.inc"
%include "font.inc"
%include "tiles.inc"
%include "video.inc"
%include "physics.inc"
%include "player.inc"
%include "enemies.inc"
%include "bullets.inc"
%include "pickups.inc"
%include "hud.inc"
