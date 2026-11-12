; bullets.inc - player bullets + enemy (kapre) projectiles

%define MAX_PB 8
%define MAX_EB 4

section .text

; ---------------------------------------------------------------------
; player_fire: muzzle from player state
player_fire:
    cmp  byte [pl_dir], 0
    jne  .l
    mov  eax, [pl_x]
    sar  eax, 8
    add  eax, 14
    jmp  .mx
.l:
    mov  eax, [pl_x]
    sar  eax, 8
    sub  eax, 6
.mx:
    mov  ebx, [pl_y]
    sar  ebx, 8
    add  ebx, 6
    xor  edi, edi
.lp:
    cmp  edi, MAX_PB
    jae  .full
    cmp  byte [pb_act + edi], 0
    je   .spawn
    inc  edi
    jmp  .lp
.spawn:
    mov  byte [pb_act + edi], 1
    shl  eax, 8
    mov  [pb_x + edi * 4], eax
    shl  ebx, 8
    mov  [pb_y + edi * 4], ebx
    mov  al, [pl_dir]
    mov  [pb_dir + edi], al
    mov  byte [pb_ttl + edi], 90
    dec  byte [pl_ammo]
    mov  byte [pl_shoot_cd], 8
    call sfx_shoot
.full:
    ret

; ---------------------------------------------------------------------
bullets_update:
    xor  edi, edi
.lp:
    cmp  edi, MAX_PB
    jae  .done
    cmp  byte [pb_act + edi], 0
    je   .next

    ; move
    mov  ebx, [pb_x + edi * 4]
    cmp  byte [pb_dir + edi], 0
    jne  .bl
    add  ebx, BULLET_SPEED
    jmp  .bm
.bl:
    sub  ebx, BULLET_SPEED
.bm:
    mov  [pb_x + edi * 4], ebx

    dec  byte [pb_ttl + edi]
    jz   .kill

    ; tile collision
    mov  eax, ebx
    sar  eax, 8
    sar  eax, 4
    mov  ecx, eax
    mov  eax, [pb_y + edi * 4]
    sar  eax, 8
    sar  eax, 4
    mov  ebx, eax
    mov  eax, ecx
    call tile_at
    call id_is_solid
    test eax, eax
    jnz  .kill

    call bullet_hit_enemy          ; edi = bullet idx -> eax 1/0
    test eax, eax
    jnz  .kill
    jmp  .next
.kill:
    mov  byte [pb_act + edi], 0
.next:
    inc  edi
    jmp  .lp
.done:
    ret

; bullet_hit_enemy: edi = bullet index -> eax = 1 if hit
bullet_hit_enemy:
    mov  eax, [pb_x + edi * 4]
    sar  eax, 8
    mov  [bh_bx], eax
    mov  eax, [pb_y + edi * 4]
    sar  eax, 8
    mov  [bh_by], eax
    mov  [bh_idx], edi

    xor  esi, esi
.el:
    cmp  esi, MAX_ENEMIES
    jae  .no
    imul eax, esi, ENEMY_SIZE
    mov  [bh_eoff], eax
    cmp  byte [enemies + E_ACT + eax], 0
    je   .en

    mov  eax, [enemies + E_X + eax]
    sar  eax, 8
    mov  [bh_ex], eax
    mov  eax, [bh_eoff]
    mov  eax, [enemies + E_Y + eax]
    sar  eax, 8
    mov  [bh_ey], eax

    ; hitbox size: boss 32, else 16
    mov  dword [bh_sz], 16
    mov  eax, [bh_eoff]
    cmp  byte [enemies + E_TYPE + eax], SPAWN_BOSS
    jne  .sz
    mov  dword [bh_sz], 32
.sz:

    ; overlap (bullet 4x3 vs enemy hitbox)
    mov  eax, [bh_ex]
    add  eax, [bh_sz]
    cmp  [bh_bx], eax
    jge  .en
    mov  eax, [bh_bx]
    add  eax, 4
    cmp  eax, [bh_ex]
    jle  .en
    mov  eax, [bh_ey]
    add  eax, [bh_sz]
    cmp  [bh_by], eax
    jge  .en
    mov  eax, [bh_by]
    add  eax, 3
    cmp  eax, [bh_ey]
    jle  .en

    ; ---- HIT ----
    mov  eax, [bh_eoff]
    mov  byte [enemies + E_HURT + eax], 10
    mov  dword [enemies + E_VX + eax], 0
    dec  byte [enemies + E_HP + eax]
    jg   .alive
    mov  byte [enemies + E_ACT + eax], 0
    call sfx_enemy_die
.alive:
    mov  eax, 1
    ret
.en:
    inc  esi
    jmp  .el
.no:
    xor  eax, eax
    ret

; ---------------------------------------------------------------------
; eb_fire: eax = enemy struct ptr; [eb_nvx]/[eb_nvy] = velocity (8.8)
eb_fire:
    push ebx
    push edi
    xor  edi, edi
.lp:
    cmp  edi, MAX_EB
    jae  .full
    cmp  byte [eb_act + edi], 0
    je   .spawn
    inc  edi
    jmp  .lp
.spawn:
    mov  byte [eb_act + edi], 1
    mov  ebx, [eax + E_X]
    add  ebx, 8 << 8
    mov  [eb_x + edi * 4], ebx
    mov  ebx, [eax + E_Y]
    add  ebx, 6 << 8
    mov  [eb_y + edi * 4], ebx
    mov  ebx, [eb_nvx]
    mov  [eb_vx + edi * 4], ebx
    mov  ebx, [eb_nvy]
    mov  [eb_vy + edi * 4], ebx
    mov  byte [eb_ttl + edi], 200
.full:
    pop  edi
    pop  ebx
    ret

; ---------------------------------------------------------------------
eb_update:
    xor  edi, edi
.lp:
    cmp  edi, MAX_EB
    jae  .done
    cmp  byte [eb_act + edi], 0
    je   .next

    mov  eax, [eb_x + edi * 4]
    add  eax, [eb_vx + edi * 4]
    mov  [eb_x + edi * 4], eax
    mov  ebx, [eb_y + edi * 4]
    add  ebx, [eb_vy + edi * 4]
    mov  [eb_y + edi * 4], ebx

    dec  byte [eb_ttl + edi]
    jz   .kill

    ; tile hit
    mov  eax, [eb_x + edi * 4]
    sar  eax, 8
    sar  eax, 4
    mov  ecx, eax
    mov  eax, ebx
    sar  eax, 8
    sar  eax, 4
    mov  ebx, eax
    mov  eax, ecx
    call tile_at
    call id_is_solid
    test eax, eax
    jnz  .kill

    ; player hit (bullet 6x6 vs player 14x16)
    mov  eax, [eb_x + edi * 4]
    sar  eax, 8
    mov  [bh_bx], eax
    mov  eax, [eb_y + edi * 4]
    sar  eax, 8
    mov  [bh_by], eax
    mov  eax, [pl_x]
    sar  eax, 8
    add  eax, 1                     ; slight inset
    mov  [bh_ex], eax
    mov  eax, [pl_y]
    sar  eax, 8
    mov  [bh_ey], eax

    mov  eax, [bh_ex]
    add  eax, 14
    cmp  [bh_bx], eax
    jge  .next
    mov  eax, [bh_bx]
    add  eax, 6
    cmp  eax, [bh_ex]
    jle  .next
    mov  eax, [bh_ey]
    add  eax, 16
    cmp  [bh_by], eax
    jge  .next
    mov  eax, [bh_by]
    add  eax, 6
    cmp  eax, [bh_ey]
    jle  .next

    push edi
    mov  eax, 15
    call player_take_damage
    pop  edi
.kill:
    mov  byte [eb_act + edi], 0
.next:
    inc  edi
    jmp  .lp
.done:
    ret

; ---------------------------------------------------------------------
; draw player bullets: 4x3 yellow-white; enemy fireballs: 6x6 orange
bullets_draw:
    xor  edi, edi
.pb:
    cmp  edi, MAX_PB
    jae  .eb0
    cmp  byte [pb_act + edi], 0
    je   .pn
    mov  eax, [pb_x + edi * 4]
    sar  eax, 8
    sub  eax, [cam_x]
    mov  ebx, [pb_y + edi * 4]
    sar  ebx, 8
    sub  ebx, [cam_y]
    mov  ecx, 4
    mov  edx, 3
    mov  esi, C_YELLOW
    call fill_rect
.pn:
    inc  edi
    jmp  .pb
.eb0:
    xor  edi, edi
.eb:
    cmp  edi, MAX_EB
    jae  .done
    cmp  byte [eb_act + edi], 0
    je   .en
    mov  eax, [eb_x + edi * 4]
    sar  eax, 8
    sub  eax, [cam_x]
    mov  ebx, [eb_y + edi * 4]
    sar  ebx, 8
    sub  ebx, [cam_y]
    mov  ecx, 6
    mov  edx, 6
    mov  esi, C_ORANGE
    call fill_rect
.en:
    inc  edi
    jmp  .eb
.done:
    ret

section .data
pb_act: times MAX_PB db 0
pb_dir: times MAX_PB db 0
pb_ttl: times MAX_PB db 0
pb_x:   times MAX_PB dd 0
pb_y:   times MAX_PB dd 0

eb_act: times MAX_EB db 0
eb_ttl: times MAX_EB db 0
eb_x:   times MAX_EB dd 0
eb_y:   times MAX_EB dd 0
eb_vx:  times MAX_EB dd 0
eb_vy:  times MAX_EB dd 0
eb_nvx: dd 0
eb_nvy: dd 0

bh_bx:  dd 0
bh_by:  dd 0
bh_ex:  dd 0
bh_ey:  dd 0
bh_eoff: dd 0
bh_idx: dd 0
bh_sz:  dd 0
