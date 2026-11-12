; enemies.inc - aswang (chaser) + kapre (turret) + tikbalang (charger)
;               + manananggal (flyer) + boss AI

%define MAX_ENEMIES 8
%define ENEMY_SIZE  32

%define E_TYPE   0
%define E_ACT    1
%define E_DIR    2
%define E_STATE  3
%define E_HP     4
%define E_TIMER  5
%define E_ANIM   6
%define E_HURT   7
%define E_X      8
%define E_Y      12
%define E_VX     16
%define E_VY     20
%define E_COOL   24

%define PATROL_SPEED 256              ; 1.0 px/t
%define CHASE_SPEED  410              ; 1.6 px/t
%define CHARGE_SPEED 768              ; 3.0 px/t (tikbalang dash)
%define KAPRE_FIRE   768              ; 3.0 px/t
%define BOSS_SPEED   192              ; 0.75 px/t

section .text

; ---------------------------------------------------------------------
enemies_init:
    xor  eax, eax
    mov  edi, enemies
    mov  ecx, MAX_ENEMIES * ENEMY_SIZE
    rep  stosb

    movzx ecx, byte [level_spawn_count]
    lea  esi, [level_spawns]
.lp:
    movzx eax, byte [esi]             ; type
    cmp  al, SPAWN_ASWANG
    je   .try
    cmp  al, SPAWN_KAPRE
    je   .try
    cmp  al, SPAWN_TIKBALANG
    je   .try
    cmp  al, SPAWN_MANANANGGAL
    je   .try
    cmp  al, SPAWN_BOSS
    jne  .next
.try:
    xor  edi, edi
.find:
    cmp  edi, MAX_ENEMIES
    jae  .next
    imul ebx, edi, ENEMY_SIZE
    cmp  byte [enemies + E_ACT + ebx], 0
    je   .mk
    inc  edi
    jmp  .find
.mk:
    imul ebx, edi, ENEMY_SIZE
    mov  [enemies + E_TYPE + ebx], al
    mov  byte [enemies + E_ACT + ebx], 1
    mov  byte [enemies + E_DIR + ebx], 1
    mov  byte [enemies + E_STATE + ebx], 0
    mov  byte [enemies + E_TIMER + ebx], 0
    mov  byte [enemies + E_ANIM + ebx], 0
    mov  byte [enemies + E_HURT + ebx], 0
    mov  byte [enemies + E_COOL + ebx], 60
    cmp  al, SPAWN_BOSS
    je   .bhp
    cmp  al, SPAWN_KAPRE
    je   .khp
    cmp  al, SPAWN_TIKBALANG
    je   .thp
    cmp  al, SPAWN_MANANANGGAL
    je   .thp
    mov  byte [enemies + E_HP + ebx], 3
    jmp  .pos
.khp:
    mov  byte [enemies + E_HP + ebx], 6
    jmp  .pos
.thp:
    mov  byte [enemies + E_HP + ebx], 4
    jmp  .pos
.bhp:
    mov  byte [enemies + E_HP + ebx], 16
.pos:
    movzx eax, word [esi + 1]
    shl  eax, 8
    mov  [enemies + E_X + ebx], eax
    movzx eax, word [esi + 3]
    shl  eax, 8
    mov  [enemies + E_Y + ebx], eax
    mov  dword [enemies + E_VX + ebx], 0
    mov  dword [enemies + E_VY + ebx], 0
.next:
    add  esi, 5
    dec  ecx
    jnz  .lp
    ret

; ---------------------------------------------------------------------
; enemies_alive: eax = number of enemies still active
enemies_alive:
    xor  eax, eax
    xor  ecx, ecx
.lp:
    cmp  ecx, MAX_ENEMIES
    jae  .done
    imul ebx, ecx, ENEMY_SIZE
    cmp  byte [enemies + E_ACT + ebx], 0
    je   .next
    inc  eax
.next:
    inc  ecx
    jmp  .lp
.done:
    ret

; ---------------------------------------------------------------------
enemies_update:
    xor  edi, edi
.lp:
    cmp  edi, MAX_ENEMIES
    jae  .done
    imul ebx, edi, ENEMY_SIZE
    cmp  byte [enemies + E_ACT + ebx], 0
    je   .next
    ; safety: fell into a pit -> despawn
    mov  eax, [enemies + E_Y + ebx]
    cmp  eax, 320 << 8
    jb   .nofall
    mov  byte [enemies + E_ACT + ebx], 0
    jmp  .next
.nofall:
    cmp  byte [enemies + E_HURT + ebx], 0
    je   .nohurt
    dec  byte [enemies + E_HURT + ebx]
.nohurt:
    mov  al, [enemies + E_TYPE + ebx]
    cmp  al, SPAWN_ASWANG
    je   .asw
    cmp  al, SPAWN_KAPRE
    je   .kap
    cmp  al, SPAWN_TIKBALANG
    je   .tik
    cmp  al, SPAWN_MANANANGGAL
    je   .mana
    call boss_update
    jmp  .next
.asw:
    call aswang_update
    jmp  .next
.kap:
    call kapre_update
    jmp  .next
.tik:
    call tikbalang_update
    jmp  .next
.mana:
    call manananggal_update
.next:
    inc  edi
    jmp  .lp
.done:
    ret

; ---------------------------------------------------------------------
; aswang: gravity + patrol (ledge/wall turn), chase when player close
; (edi = enemy index; preserved across all calls)
aswang_update:
    imul ebx, edi, ENEMY_SIZE
    lea  ebx, [enemies + E_X + ebx]
    imul ecx, edi, ENEMY_SIZE
    lea  ecx, [enemies + E_Y + ecx]
    imul edx, edi, ENEMY_SIZE
    lea  edx, [enemies + E_VX + edx]
    imul esi, edi, ENEMY_SIZE
    lea  esi, [enemies + E_VY + esi]
    call phys_step
    mov  [en_ground], al

    imul ebx, edi, ENEMY_SIZE
    cmp  byte [enemies + E_HURT + ebx], 0
    jne  .finish                     ; stun: no AI while flashing

    ; dx / dy to player (px)
    mov  eax, [pl_x]
    sar  eax, 8
    mov  ecx, [enemies + E_X + ebx]
    sar  ecx, 8
    sub  eax, ecx
    mov  [en_dx], eax
    mov  ecx, [pl_y]
    sar  ecx, 8
    mov  edx, [enemies + E_Y + ebx]
    sar  edx, 8
    sub  ecx, edx
    mov  [en_dy], ecx

    ; |dx| < 256 and |dy| < 64 -> chase
    mov  edx, eax
    neg  edx
    test eax, eax
    cmovl eax, edx
    cmp  eax, 256
    jge  .patrol
    mov  eax, [en_dy]
    mov  edx, eax
    neg  edx
    test eax, eax
    cmovl eax, edx
    cmp  eax, 64
    jge  .patrol

    ; chase: vx = sign(dx)*CHASE_SPEED, face dx
    mov  dword [enemies + E_VX + ebx], CHASE_SPEED
    cmp  dword [en_dx], 0
    jge  .cvx
    neg  dword [enemies + E_VX + ebx]
.cvx:
    mov  byte [enemies + E_DIR + ebx], 0
    cmp  dword [en_dx], 0
    jg   .cface
    mov  byte [enemies + E_DIR + ebx], 1
.cface:
    ; never walk off ledges / into walls while chasing
    call aswang_blocked
    test eax, eax
    jz   .finish
    imul ebx, edi, ENEMY_SIZE
    mov  dword [enemies + E_VX + ebx], 0
    jmp  .finish

.patrol:
    mov  dword [enemies + E_VX + ebx], PATROL_SPEED
    cmp  byte [enemies + E_DIR + ebx], 0
    je   .pdir
    neg  dword [enemies + E_VX + ebx]
.pdir:
    cmp  byte [en_ground], 1
    jne  .finish
    call aswang_blocked              ; clobbers eax/ebx/ecx
    test eax, eax
    jz   .finish
    imul ebx, edi, ENEMY_SIZE
    xor  byte [enemies + E_DIR + ebx], 1

.finish:
    call enemy_contact
    imul ebx, edi, ENEMY_SIZE
    inc  byte [enemies + E_ANIM + ebx]
    ret

; aswang_blocked: edi = idx -> eax=1 if wall ahead OR ledge ahead
aswang_blocked:
    ; wall: tile at (edge ahead, torso)
    imul ebx, edi, ENEMY_SIZE
    mov  eax, [enemies + E_X + ebx]
    sar  eax, 8
    cmp  byte [enemies + E_DIR + ebx], 0
    jne  .wl
    add  eax, 16
    jmp  .wc
.wl:
    sub  eax, 1
.wc:
    sar  eax, 4
    mov  ecx, eax
    mov  eax, [enemies + E_Y + ebx]
    sar  eax, 8
    add  eax, 8
    sar  eax, 4
    mov  ebx, eax
    mov  eax, ecx
    call tile_at
    call id_is_solid
    test eax, eax
    jnz  .yes

    ; ledge: tile below feet ahead must be solid
    ; (probe = y + body height + 1; works for 16px and 32px bodies)
    imul ebx, edi, ENEMY_SIZE
    mov  eax, [enemies + E_X + ebx]
    sar  eax, 8
    cmp  byte [enemies + E_DIR + ebx], 0
    jne  .ll
    add  eax, 16
    jmp  .lc
.ll:
    sub  eax, 1
.lc:
    sar  eax, 4
    mov  ecx, eax
    mov  eax, [enemies + E_Y + ebx]
    sar  eax, 8
    mov  ebx, [phys_h]
    inc  ebx
    add  eax, ebx
    sar  eax, 4
    mov  ebx, eax
    mov  eax, ecx
    call tile_at
    call id_is_solid
    test eax, eax
    jz   .yes
    xor  eax, eax
    ret
.yes:
    mov  eax, 1
    ret

; ---------------------------------------------------------------------
; kapre: static turret, lobs aimed fireballs on cooldown
kapre_update:
    imul ebx, edi, ENEMY_SIZE
    lea  ebx, [enemies + E_X + ebx]
    imul ecx, edi, ENEMY_SIZE
    lea  ecx, [enemies + E_Y + ecx]
    imul edx, edi, ENEMY_SIZE
    lea  edx, [enemies + E_VX + edx]
    imul esi, edi, ENEMY_SIZE
    lea  esi, [enemies + E_VY + esi]
    call phys_step

    imul ebx, edi, ENEMY_SIZE
    cmp  byte [enemies + E_HURT + ebx], 0
    jne  .finish

    cmp  byte [enemies + E_COOL + ebx], 0
    je   .check
    dec  byte [enemies + E_COOL + ebx]
    jmp  .finish
.check:
    ; in range? |dx|<288, |dy|<80
    mov  eax, [pl_x]
    sar  eax, 8
    mov  ecx, [enemies + E_X + ebx]
    sar  ecx, 8
    sub  eax, ecx
    mov  [en_dx], eax
    mov  ecx, [pl_y]
    sar  ecx, 8
    mov  edx, [enemies + E_Y + ebx]
    sar  edx, 8
    sub  ecx, edx
    mov  [en_dy], ecx

    mov  edx, eax
    neg  edx
    test eax, eax
    cmovl eax, edx
    cmp  eax, 288
    jge  .cool15
    mov  eax, [en_dy]
    mov  edx, eax
    neg  edx
    test eax, eax
    cmovl eax, edx
    cmp  eax, 80
    jge  .cool15

    ; vx = sign(dx)*KAPRE_FIRE (dir fallback when dx==0)
    mov  eax, KAPRE_FIRE
    cmp  dword [en_dx], 0
    jg   .vxok
    jl   .vneg
    cmp  byte [enemies + E_DIR + ebx], 0
    je   .vxok
.vneg:
    neg  eax
.vxok:
    mov  [eb_nvx], eax

    ; vy = vx*dy/dx (keeps vertical direction aligned with dy), clamp +-512
    mov  eax, [en_dx]
    test eax, eax
    jnz  .dv
    xor  edx, edx
    jmp  .vydone
.dv:
    mov  eax, [eb_nvx]
    imul dword [en_dy]              ; edx:eax = vx * dy
    mov  ecx, [en_dx]
    idiv ecx                        ; eax = vx*dy/dx
    cmp  eax, 512
    jle  .c1
    mov  eax, 512
    jmp  .vydone
.c1:
    cmp  eax, -512
    jge  .vydone
    mov  eax, -512
.vydone:
    mov  [eb_nvy], eax

    lea  eax, [enemies + E_X + ebx]
    call eb_fire                      ; preserves ebx/esi/edi
    mov  byte [enemies + E_COOL + ebx], 120
    cmp  dword [en_dx], 0
    jg   .face0
    mov  byte [enemies + E_DIR + ebx], 1
    jmp  .finish
.face0:
    mov  byte [enemies + E_DIR + ebx], 0
    jmp  .finish

.cool15:
    mov  byte [enemies + E_COOL + ebx], 15
.finish:
    call enemy_contact
    imul ebx, edi, ENEMY_SIZE
    inc  byte [enemies + E_ANIM + ebx]
    ret

; ---------------------------------------------------------------------
; tikbalang: patrol (ledge/wall turn), wind-up charge when player close
; E_STATE: 0 patrol, 1 charging, 2 resting
tikbalang_update:
    imul ebx, edi, ENEMY_SIZE
    lea  ebx, [enemies + E_X + ebx]
    imul ecx, edi, ENEMY_SIZE
    lea  ecx, [enemies + E_Y + ecx]
    imul edx, edi, ENEMY_SIZE
    lea  edx, [enemies + E_VX + edx]
    imul esi, edi, ENEMY_SIZE
    lea  esi, [enemies + E_VY + esi]
    call phys_step
    mov  [en_ground], al

    imul ebx, edi, ENEMY_SIZE
    cmp  byte [enemies + E_HURT + ebx], 0
    jne  .finish

    ; dx / dy to player (px)
    mov  eax, [pl_x]
    sar  eax, 8
    mov  ecx, [enemies + E_X + ebx]
    sar  ecx, 8
    sub  eax, ecx
    mov  [en_dx], eax
    mov  ecx, [pl_y]
    sar  ecx, 8
    mov  edx, [enemies + E_Y + ebx]
    sar  edx, 8
    sub  ecx, edx
    mov  [en_dy], ecx

    cmp  byte [enemies + E_STATE + ebx], 1
    je   .charging
    cmp  byte [enemies + E_STATE + ebx], 2
    je   .rest

    ; ---- state 0: patrol ----
    mov  dword [enemies + E_VX + ebx], PATROL_SPEED
    cmp  byte [enemies + E_DIR + ebx], 0
    je   .pdir
    neg  dword [enemies + E_VX + ebx]
.pdir:
    cmp  byte [en_ground], 1
    jne  .detect
    call aswang_blocked              ; clobbers eax/ebx/ecx
    test eax, eax
    jz   .detect
    imul ebx, edi, ENEMY_SIZE
    xor  byte [enemies + E_DIR + ebx], 1
.detect:
    imul ebx, edi, ENEMY_SIZE
    mov  eax, [en_dx]
    mov  edx, eax
    neg  edx
    test eax, eax
    cmovl eax, edx
    cmp  eax, 256
    jge  .finish
    mov  eax, [en_dy]
    mov  edx, eax
    neg  edx
    test eax, eax
    cmovl eax, edx
    cmp  eax, 64
    jge  .finish
    ; -> charge toward player
    mov  byte [enemies + E_STATE + ebx], 1
    mov  byte [enemies + E_TIMER + ebx], 45
    mov  byte [enemies + E_DIR + ebx], 0
    cmp  dword [en_dx], 0
    jg   .cface
    mov  byte [enemies + E_DIR + ebx], 1
.cface:
    mov  dword [enemies + E_VX + ebx], CHARGE_SPEED
    cmp  byte [enemies + E_DIR + ebx], 0
    je   .finish
    neg  dword [enemies + E_VX + ebx]
    jmp  .finish

.charging:
    cmp  byte [en_ground], 1
    jne  .ctick
    call aswang_blocked              ; clobbers ebx
    test eax, eax
    jz   .ctick
    imul ebx, edi, ENEMY_SIZE
    mov  byte [enemies + E_STATE + ebx], 2
    mov  byte [enemies + E_COOL + ebx], 40
    mov  dword [enemies + E_VX + ebx], 0
    jmp  .finish
.ctick:
    imul ebx, edi, ENEMY_SIZE
    dec  byte [enemies + E_TIMER + ebx]
    jnz  .finish
    mov  byte [enemies + E_STATE + ebx], 2
    mov  byte [enemies + E_COOL + ebx], 40
    mov  dword [enemies + E_VX + ebx], 0
    jmp  .finish

.rest:
    mov  dword [enemies + E_VX + ebx], 0
    dec  byte [enemies + E_COOL + ebx]
    jnz  .finish
    mov  byte [enemies + E_STATE + ebx], 0

.finish:
    call enemy_contact
    imul ebx, edi, ENEMY_SIZE
    inc  byte [enemies + E_ANIM + ebx]
    ret

; ---------------------------------------------------------------------
; manananggal: flying - sine hover + slow chase, no gravity/collision
manananggal_update:
    imul ebx, edi, ENEMY_SIZE
    cmp  byte [enemies + E_HURT + ebx], 0
    jne  .finish                    ; stunned: freeze in air

    ; dx / dy to player (px)
    mov  eax, [pl_x]
    sar  eax, 8
    mov  ecx, [enemies + E_X + ebx]
    sar  ecx, 8
    sub  eax, ecx
    mov  [en_dx], eax
    mov  ecx, [pl_y]
    sar  ecx, 8
    mov  edx, [enemies + E_Y + ebx]
    sar  edx, 8
    sub  ecx, edx
    mov  [en_dy], ecx

    ; chase if |dx| < 288 and |dy| < 160
    mov  eax, [en_dx]
    mov  edx, eax
    neg  edx
    test eax, eax
    cmovl eax, edx
    cmp  eax, 288
    jge  .hover
    mov  eax, [en_dy]
    mov  edx, eax
    neg  edx
    test eax, eax
    cmovl eax, edx
    cmp  eax, 160
    jge  .hover

    ; chase: vx = sign(dx)*300, vy = sign(dy)*200, face player
    mov  dword [enemies + E_VX + ebx], 300
    cmp  dword [en_dx], 0
    jge  .mvx
    neg  dword [enemies + E_VX + ebx]
.mvx:
    mov  dword [enemies + E_VY + ebx], 200
    cmp  dword [en_dy], 0
    jge  .mvy
    neg  dword [enemies + E_VY + ebx]
.mvy:
    mov  byte [enemies + E_DIR + ebx], 0
    cmp  dword [en_dx], 0
    jg   .move
    mov  byte [enemies + E_DIR + ebx], 1
    jmp  .move

.hover:
    ; drift slowly toward player on x, bob on y (bit6 of anim)
    mov  dword [enemies + E_VX + ebx], 64
    cmp  dword [en_dx], 0
    jge  .hvx
    neg  dword [enemies + E_VX + ebx]
.hvx:
    mov  dword [enemies + E_VY + ebx], 96
    test byte [enemies + E_ANIM + ebx], 0x40
    jz   .move
    neg  dword [enemies + E_VY + ebx]

.move:
    ; x += vx (8.8), clamp to map
    mov  eax, [enemies + E_X + ebx]
    add  eax, [enemies + E_VX + ebx]
    test eax, eax
    jns  .xok
    xor  eax, eax
    jmp  .xset
.xok:
    cmp  eax, (MAP_W * 16 - 16) << 8
    jle  .xset
    mov  eax, (MAP_W * 16 - 16) << 8
.xset:
    mov  [enemies + E_X + ebx], eax
    ; y += vy (8.8), clamp 16..300 px (avoids pit-despawn)
    mov  eax, [enemies + E_Y + ebx]
    add  eax, [enemies + E_VY + ebx]
    cmp  eax, 16 << 8
    jge  .yok
    mov  eax, 16 << 8
    jmp  .yset
.yok:
    cmp  eax, 300 << 8
    jle  .yset
    mov  eax, 300 << 8
.yset:
    mov  [enemies + E_Y + ebx], eax

.finish:
    call enemy_contact
    imul ebx, edi, ENEMY_SIZE
    inc  byte [enemies + E_ANIM + ebx]
    ret

; ---------------------------------------------------------------------
; boss: ground tank - walks at player, lobs aimed fireballs, 32x32 body
boss_update:
    imul ebx, edi, ENEMY_SIZE
    lea  ebx, [enemies + E_X + ebx]
    imul ecx, edi, ENEMY_SIZE
    lea  ecx, [enemies + E_Y + ecx]
    imul edx, edi, ENEMY_SIZE
    lea  edx, [enemies + E_VX + edx]
    imul esi, edi, ENEMY_SIZE
    lea  esi, [enemies + E_VY + esi]
    mov  dword [phys_w], 16
    mov  dword [phys_h], 32
    call phys_step
    mov  [en_ground], al

    imul ebx, edi, ENEMY_SIZE
    cmp  byte [enemies + E_HURT + ebx], 0
    jne  .finish

    ; dx / dy to player (px)
    mov  eax, [pl_x]
    sar  eax, 8
    mov  ecx, [enemies + E_X + ebx]
    sar  ecx, 8
    sub  eax, ecx
    mov  [en_dx], eax
    mov  ecx, [pl_y]
    sar  ecx, 8
    mov  edx, [enemies + E_Y + ebx]
    sar  edx, 8
    sub  ecx, edx
    mov  [en_dy], ecx

    ; walk toward player when |dx| > 48
    mov  eax, [en_dx]
    mov  edx, eax
    neg  edx
    test eax, eax
    cmovl eax, edx
    cmp  eax, 48
    jle  .nostep
    mov  dword [enemies + E_VX + ebx], BOSS_SPEED
    cmp  dword [en_dx], 0
    jge  .bvx
    neg  dword [enemies + E_VX + ebx]
.bvx:
    mov  byte [enemies + E_DIR + ebx], 0
    cmp  dword [en_dx], 0
    jg   .bface
    mov  byte [enemies + E_DIR + ebx], 1
.bface:
    cmp  byte [en_ground], 1
    jne  .nocool
    call aswang_blocked              ; clobbers eax/ebx/ecx
    imul ebx, edi, ENEMY_SIZE
    test eax, eax
    jz   .nocool
    mov  dword [enemies + E_VX + ebx], 0
    jmp  .nocool
.nostep:
    mov  dword [enemies + E_VX + ebx], 0
.nocool:
    ; fire cooldown
    cmp  byte [enemies + E_COOL + ebx], 0
    je   .check
    dec  byte [enemies + E_COOL + ebx]
    jmp  .finish
.check:
    ; in range? |dx| < 384, |dy| < 96
    mov  eax, [en_dx]
    mov  edx, eax
    neg  edx
    test eax, eax
    cmovl eax, edx
    cmp  eax, 384
    jge  .cool15
    mov  eax, [en_dy]
    mov  edx, eax
    neg  edx
    test eax, eax
    cmovl eax, edx
    cmp  eax, 96
    jge  .cool15

    ; aimed fireball (same math as kapre)
    mov  eax, KAPRE_FIRE
    cmp  dword [en_dx], 0
    jg   .fvx
    jl   .fvn
    cmp  byte [enemies + E_DIR + ebx], 0
    je   .fvx
.fvn:
    neg  eax
.fvx:
    mov  [eb_nvx], eax
    mov  eax, [en_dx]
    test eax, eax
    jnz  .fdv
    xor  edx, edx
    jmp  .fvy
.fdv:
    mov  eax, [eb_nvx]
    imul dword [en_dy]              ; edx:eax = vx * dy
    mov  ecx, [en_dx]
    idiv ecx                        ; eax = vx*dy/dx
    cmp  eax, 512
    jle  .fc1
    mov  eax, 512
    jmp  .fvy
.fc1:
    cmp  eax, -512
    jge  .fvy
    mov  eax, -512
.fvy:
    mov  [eb_nvy], eax
    lea  eax, [enemies + E_X + ebx]
    call eb_fire                      ; preserves ebx/esi/edi
    mov  byte [enemies + E_COOL + ebx], 90
    cmp  dword [en_dx], 0
    jg   .fface0
    mov  byte [enemies + E_DIR + ebx], 1
    jmp  .finish
.fface0:
    mov  byte [enemies + E_DIR + ebx], 0
    jmp  .finish

.cool15:
    mov  byte [enemies + E_COOL + ebx], 15
.finish:
    mov  dword [phys_w], 16          ; restore body size for others
    mov  dword [phys_h], 16
    call enemy_contact
    imul ebx, edi, ENEMY_SIZE
    inc  byte [enemies + E_ANIM + ebx]
    ret

; ---------------------------------------------------------------------
; enemy_contact: edi = idx -> player damage if overlapping
enemy_contact:
    imul ebx, edi, ENEMY_SIZE
    mov  eax, [enemies + E_X + ebx]
    sar  eax, 8
    mov  [bh_ex], eax
    mov  eax, [enemies + E_Y + ebx]
    sar  eax, 8
    mov  [bh_ey], eax
    ; hitbox size: boss 32x32, everyone else 16x16
    mov  dword [bh_sz], 16
    cmp  byte [enemies + E_TYPE + ebx], SPAWN_BOSS
    jne  .sz
    mov  dword [bh_sz], 32
.sz:
    mov  eax, [pl_x]
    sar  eax, 8
    add  eax, 1
    mov  [bh_bx], eax
    mov  eax, [pl_y]
    sar  eax, 8
    mov  [bh_by], eax

    mov  eax, [bh_ex]
    add  eax, [bh_sz]
    cmp  [bh_bx], eax
    jge  .no
    mov  eax, [bh_bx]
    add  eax, 14
    cmp  eax, [bh_ex]
    jle  .no
    mov  eax, [bh_ey]
    add  eax, [bh_sz]
    cmp  [bh_by], eax
    jge  .no
    mov  eax, [bh_by]
    add  eax, 16
    cmp  eax, [bh_ey]
    jle  .no

    imul ebx, edi, ENEMY_SIZE
    mov  eax, 10                     ; aswang / manananggal
    cmp  byte [enemies + E_TYPE + ebx], SPAWN_ASWANG
    je   .d
    cmp  byte [enemies + E_TYPE + ebx], SPAWN_MANANANGGAL
    je   .d
    cmp  byte [enemies + E_TYPE + ebx], SPAWN_BOSS
    je   .dboss
    mov  eax, 15                     ; kapre / tikbalang
    jmp  .d
.dboss:
    mov  eax, 20
.d:
    call player_take_damage
.no:
    ret

; ---------------------------------------------------------------------
enemies_draw:
    mov  dword [en_idx], 0
.lp:
    mov  edi, [en_idx]
    cmp  edi, MAX_ENEMIES
    jae  .done
    imul ebx, edi, ENEMY_SIZE
    cmp  byte [enemies + E_ACT + ebx], 0
    je   .next
    cmp  byte [enemies + E_HURT + ebx], 0
    je   .show
    test byte [enemies + E_HURT + ebx], 1
    jnz  .next
.show:
    ; anim frame -> eax 0/1
    movzx eax, byte [enemies + E_ANIM + ebx]
    shr  eax, 4
    and  eax, 1
    mov  esi, 16
    mov  edi, 16
    cmp  byte [enemies + E_TYPE + ebx], SPAWN_KAPRE
    je   .kap
    cmp  byte [enemies + E_TYPE + ebx], SPAWN_TIKBALANG
    je   .tik
    cmp  byte [enemies + E_TYPE + ebx], SPAWN_MANANANGGAL
    je   .mana
    cmp  byte [enemies + E_TYPE + ebx], SPAWN_BOSS
    je   .boss
    test eax, eax
    jz   .s1
    mov  eax, spr_aswang2
    jmp  .prep
.s1:
    mov  eax, spr_aswang1
    jmp  .prep
.kap:
    test eax, eax
    jz   .k1
    mov  eax, spr_kapre2
    jmp  .prep
.k1:
    mov  eax, spr_kapre1
    jmp  .prep
.tik:
    test eax, eax
    jz   .t1
    mov  eax, spr_tikbalang2
    jmp  .prep
.t1:
    mov  eax, spr_tikbalang1
    jmp  .prep
.mana:
    test eax, eax
    jz   .m1
    mov  eax, spr_manananggal2
    jmp  .prep
.m1:
    mov  eax, spr_manananggal1
    jmp  .prep
.boss:
    mov  esi, 32
    mov  edi, 32
    test eax, eax
    jz   .b1
    mov  eax, spr_boss2
    jmp  .prep
.b1:
    mov  eax, spr_boss1
.prep:
    movzx edx, byte [enemies + E_DIR + ebx]
    mov  ecx, [enemies + E_Y + ebx]
    sar  ecx, 8
    sub  ecx, [cam_y]
    mov  ebx, [enemies + E_X + ebx]
    sar  ebx, 8
    sub  ebx, [cam_x]
    call draw_ascii
.next:
    inc  dword [en_idx]
    jmp  .lp
.done:
    ret

section .data
enemies: times MAX_ENEMIES * ENEMY_SIZE db 0
en_dx:   dd 0
en_dy:   dd 0
en_adx:  dd 0
en_ground: db 0
en_idx:  dd 0
