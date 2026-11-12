; player.inc - player state, input, physics, drawing

section .text

; ---------------------------------------------------------------------
player_init:                        ; respawn (keeps hp/lives/ammo/key)
    movzx eax, word [level_player_spawn]
    shl  eax, 8
    mov  [pl_x], eax
    movzx eax, word [level_player_spawn + 2]
    shl  eax, 8
    mov  [pl_y], eax
    mov  dword [pl_vx], 0
    mov  dword [pl_vy], 0
    mov  byte [pl_dir], 0
    mov  byte [pl_on_ground], 0
    mov  byte [pl_invuln], 60
    mov  byte [pl_anim], 0
    mov  byte [pl_shoot_cd], 0
    ret

player_restart:                     ; brand new game
    mov  dword [pl_hp], 100
    mov  byte [pl_lives], 3
    mov  byte [pl_ammo], 20
    mov  byte [pl_has_key], 0
    call player_init
    ret

; ---------------------------------------------------------------------
player_update:
    ; horizontal input
    xor  eax, eax
    cmp  byte [key_state + KEY_RIGHT], 1
    jne  .nl
    mov  eax, RUN_SPEED
    mov  byte [pl_dir], 0
    jmp  .hs
.nl:
    cmp  byte [key_state + KEY_LEFT], 1
    jne  .hs
    mov  eax, -RUN_SPEED
    mov  byte [pl_dir], 1
.hs:
    mov  [pl_vx], eax

    ; jump (held space auto-rejumps on landing)
    cmp  byte [key_state + KEY_SPACE], 1
    jne  .nj
    cmp  byte [pl_on_ground], 1
    jne  .nj
    mov  dword [pl_vy], JUMP_V
    mov  byte [pl_on_ground], 0
    call sfx_jump
.nj:
    ; shoot cooldown
    cmp  byte [pl_shoot_cd], 0
    je   .cd0
    dec  byte [pl_shoot_cd]
.cd0:
    cmp  byte [key_state + KEY_CTRL], 1
    jne  .ns
    cmp  byte [pl_shoot_cd], 0
    jne  .ns
    cmp  byte [pl_ammo], 0
    je   .ns
    call player_fire
.ns:
    ; physics
    mov  dword [phys_w], 16
    mov  dword [phys_h], 16
    mov  ebx, pl_x
    mov  ecx, pl_y
    mov  edx, pl_vx
    mov  esi, pl_vy
    call phys_step
    mov  [pl_on_ground], al

    ; timers
    cmp  byte [pl_invuln], 0
    je   .iv0
    dec  byte [pl_invuln]
.iv0:
    inc  byte [pl_anim]
    ret

; ---------------------------------------------------------------------
; player_take_damage: eax = damage
player_take_damage:
    cmp  byte [pl_invuln], 0
    jne  .no
    cmp  dword [game_state], ST_PLAY
    jne  .no
    sub  dword [pl_hp], eax
    jg   .ok
    mov  dword [pl_hp], 0
.ok:
    mov  byte [pl_invuln], 60
    call sfx_hurt
.no:
    ret

; ---------------------------------------------------------------------
player_draw:
    ; blink while invulnerable
    cmp  byte [pl_invuln], 0
    je   .draw
    test byte [pl_invuln], 1
    jnz  .done
.draw:
    mov  eax, spr_player_idle
    cmp  byte [pl_on_ground], 1
    je   .ground
    mov  eax, spr_player_jump
    jmp  .got
.ground:
    cmp  byte [pl_shoot_cd], 4
    ja   .shoot
    cmp  dword [pl_vx], 0
    je   .got
    movzx ebx, byte [pl_anim]
    shr  ebx, 3
    and  ebx, 1
    test ebx, ebx
    jz   .got
    mov  eax, spr_player_run2
    jmp  .got
.shoot:
    mov  eax, spr_player_shoot
.got:
    movzx edx, byte [pl_dir]        ; flip when facing left
    mov  esi, 16
    mov  edi, 16
    mov  ebx, [pl_x]
    sar  ebx, 8
    sub  ebx, [cam_x]
    mov  ecx, [pl_y]
    sar  ecx, 8
    sub  ecx, [cam_y]
    call draw_ascii
.done:
    ret

section .data
pl_x:          dd 0                ; 8.8 fixed point
pl_y:          dd 0
pl_vx:         dd 0
pl_vy:         dd 0
pl_dir:        db 0                ; 0 = face right, 1 = face left
pl_on_ground:  db 0
pl_hp:         dd 100
pl_lives:      db 3
pl_ammo:       db 20
pl_has_key:    db 0
pl_invuln:     db 0
pl_anim:       db 0
pl_shoot_cd:   db 0
