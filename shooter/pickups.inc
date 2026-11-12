; pickups.inc - health / ammo / key items
;
; stride P_SIZE=16: 0 P_ACT db, 1 P_TYPE db, 4 P_X dd, 8 P_Y dd

%define MAX_PICKUPS 8
%define P_SIZE 16
%define P_ACT  0
%define P_TYPE 1
%define P_X    4
%define P_Y    8

section .text

; ---------------------------------------------------------------------
pickups_init:
    xor  eax, eax
    mov  edi, pickups
    mov  ecx, MAX_PICKUPS * P_SIZE
    rep  stosb

    movzx ecx, byte [level_spawn_count]
    lea  esi, [level_spawns]
.lp:
    movzx eax, byte [esi]
    cmp  al, SPAWN_HEALTH
    jb   .next
    cmp  al, SPAWN_KEY
    ja   .next
    ; type 3,4,5
    xor  edi, edi
.find:
    cmp  edi, MAX_PICKUPS
    jae  .next
    imul ebx, edi, P_SIZE
    cmp  byte [pickups + P_ACT + ebx], 0
    je   .mk
    inc  edi
    jmp  .find
.mk:
    imul ebx, edi, P_SIZE
    mov  [pickups + P_TYPE + ebx], al
    mov  byte [pickups + P_ACT + ebx], 1
    movzx eax, word [esi + 1]
    shl  eax, 8
    mov  [pickups + P_X + ebx], eax
    movzx eax, word [esi + 3]
    shl  eax, 8
    mov  [pickups + P_Y + ebx], eax
.next:
    add  esi, 5
    dec  ecx
    jnz  .lp
    ret

; ---------------------------------------------------------------------
pickups_update:
    mov  dword [pk_idx], 0
.lp:
    mov  edi, [pk_idx]
    cmp  edi, MAX_PICKUPS
    jae  .done
    imul ebx, edi, P_SIZE
    cmp  byte [pickups + P_ACT + ebx], 0
    je   .next

    ; AABB vs player (item 16x16 vs player 16x16)
    mov  eax, [pickups + P_X + ebx]
    sar  eax, 8
    mov  [bh_ex], eax
    mov  eax, [pickups + P_Y + ebx]
    sar  eax, 8
    mov  [bh_ey], eax
    mov  eax, [pl_x]
    sar  eax, 8
    mov  [bh_bx], eax
    mov  eax, [pl_y]
    sar  eax, 8
    mov  [bh_by], eax

    mov  eax, [bh_ex]
    add  eax, 16
    cmp  [bh_bx], eax
    jge  .next
    mov  eax, [bh_bx]
    add  eax, 16
    cmp  eax, [bh_ex]
    jle  .next
    mov  eax, [bh_ey]
    add  eax, 16
    cmp  [bh_by], eax
    jge  .next
    mov  eax, [bh_by]
    add  eax, 16
    cmp  eax, [bh_ey]
    jle  .next

    ; ---- touched ----
    imul ebx, edi, P_SIZE
    movzx eax, byte [pickups + P_TYPE + ebx]
    cmp  al, SPAWN_HEALTH
    je   .health
    cmp  al, SPAWN_AMMO
    je   .ammo
    ; KEY
    mov  byte [pl_has_key], 1
    mov  byte [pickups + P_ACT + ebx], 0
    call sfx_pickup
    jmp  .next
.health:
    cmp  dword [pl_hp], 100
    jge  .next                      ; full hp: leave item
    add  dword [pl_hp], 25
    cmp  dword [pl_hp], 100
    jle  .hok
    mov  dword [pl_hp], 100
.hok:
    mov  byte [pickups + P_ACT + ebx], 0
    call sfx_pickup
    jmp  .next
.ammo:
    movzx eax, byte [pl_ammo]
    cmp  eax, 99
    jge  .next
    add  byte [pl_ammo], 10
    movzx eax, byte [pl_ammo]
    cmp  eax, 99
    jle  .aok
    mov  byte [pl_ammo], 99
.aok:
    mov  byte [pickups + P_ACT + ebx], 0
    call sfx_pickup
.next:
    inc  dword [pk_idx]
    jmp  .lp
.done:
    ret

; ---------------------------------------------------------------------
pickups_draw:
    mov  dword [pk_idx], 0
.lp:
    mov  edi, [pk_idx]
    cmp  edi, MAX_PICKUPS
    jae  .done
    imul ebx, edi, P_SIZE
    cmp  byte [pickups + P_ACT + ebx], 0
    je   .next
    movzx eax, byte [pickups + P_TYPE + ebx]
    cmp  al, SPAWN_HEALTH
    je   .hp
    cmp  al, SPAWN_AMMO
    je   .am
    mov  eax, spr_key
    jmp  .draw
.hp:
    mov  eax, spr_health
    jmp  .draw
.am:
    mov  eax, spr_ammo
.draw:
    xor  edx, edx                    ; no flip
    mov  esi, 16
    mov  edi, 16
    mov  ecx, [pickups + P_Y + ebx]
    sar  ecx, 8
    sub  ecx, [cam_y]
    mov  ebx, [pickups + P_X + ebx]
    sar  ebx, 8
    sub  ebx, [cam_x]
    call draw_ascii
.next:
    inc  dword [pk_idx]
    jmp  .lp
.done:
    ret

section .data
pickups: times MAX_PICKUPS * P_SIZE db 0
pk_idx:  dd 0
