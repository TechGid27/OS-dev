; hud.inc - status bar (HP / AMMO / LIVES / KEY)
;
; layout (16px bar, everything on the same y4..11 band):
;   x2  "HP" | bar x19..121  | sep x128
;   x136 "AMMO" + value right-aligned at x192  | sep x200
;   x208 "LIVES" + value right-aligned at x264 | sep x272
;   x280 "KEY" (yellow = held, gray = empty)

section .text

; ---------------------------------------------------------------------
; draw_num: eax = value (0..999), ebx = right edge x, ecx = y, edx = color
; right-aligned so digits never drift out of their slot
draw_num:
    mov  [dn_val], eax
    mov  [dn_x], ebx
    mov  [dn_y], ecx
    mov  [dn_col], edx
    mov  edi, dn_buf + 4
    mov  byte [edi], 0
    mov  eax, [dn_val]
.dl:
    xor  edx, edx
    mov  ecx, 10
    div  ecx                         ; eax = quot, edx = rem
    add  dl, '0'
    dec  edi
    mov  [edi], dl
    test eax, eax
    jnz  .dl
    mov  esi, edi
    mov  eax, dn_buf + 4
    sub  eax, edi                    ; digit count
    shl  eax, 3                      ; * 8 px
    mov  ebx, [dn_x]
    sub  ebx, eax                    ; x = right edge - width
    mov  ecx, [dn_y]
    mov  edx, [dn_col]
    call draw_text
    ret

; ---------------------------------------------------------------------
hud_draw:
    ; panel body
    xor  eax, eax
    xor  ebx, ebx
    mov  ecx, SCREEN_W
    mov  edx, 16
    mov  esi, C_DARK0 + 1
    call fill_rect
    ; top highlight / bottom shadow
    xor  eax, eax
    xor  ebx, ebx
    mov  ecx, SCREEN_W
    mov  edx, 1
    mov  esi, C_DARK0 + 3
    call fill_rect
    xor  eax, eax
    mov  ebx, 15
    mov  ecx, SCREEN_W
    mov  edx, 1
    mov  esi, C_BLACK
    call fill_rect

    ; section separators at x=128,200,272 (y4..11)
    mov  esi, C_GRAYD
    mov  eax, 128
    mov  ebx, 4
    mov  ecx, 1
    mov  edx, 8
    call fill_rect
    mov  esi, C_GRAYD
    mov  eax, 200
    mov  ebx, 4
    mov  ecx, 1
    mov  edx, 8
    call fill_rect
    mov  esi, C_GRAYD
    mov  eax, 272
    mov  ebx, 4
    mov  ecx, 1
    mov  edx, 8
    call fill_rect

    ; ---- HP ----
    mov  esi, str_hp
    mov  ebx, 2
    mov  ecx, 4
    mov  edx, C_WHITE
    call draw_text
    ; hp bar: black border, red bg, green fg (x19..121, y3..12)
    mov  eax, 19
    mov  ebx, 3
    mov  ecx, 102
    mov  edx, 10
    mov  esi, C_BLACK
    call fill_rect
    mov  eax, 20
    mov  ebx, 4
    mov  ecx, 100
    mov  edx, 8
    mov  esi, C_HPBG + 1
    call fill_rect
    mov  eax, [pl_hp]
    test eax, eax
    jz   .nofill
    mov  ecx, eax                    ; w = hp (0..100)
    mov  eax, 20
    mov  ebx, 4
    mov  edx, 8
    mov  esi, C_HPFG + 2
    call fill_rect
.nofill:

    ; ---- AMMO ----
    mov  esi, str_ammo
    mov  ebx, 136
    mov  ecx, 4
    mov  edx, C_WHITE
    call draw_text
    movzx eax, byte [pl_ammo]
    mov  ebx, 192
    mov  ecx, 4
    mov  edx, C_YELLOW
    call draw_num

    ; ---- LIVES ----
    mov  esi, str_lives
    mov  ebx, 208
    mov  ecx, 4
    mov  edx, C_WHITE
    call draw_text
    movzx eax, byte [pl_lives]
    mov  ebx, 264
    mov  ecx, 4
    mov  edx, C_CYAN
    call draw_num

    ; ---- KEY ---- (always in place; gray = not held)
    mov  esi, str_key
    mov  ebx, 280
    mov  ecx, 4
    mov  edx, C_GRAYD
    cmp  byte [pl_has_key], 1
    jne  .drawkey
    mov  edx, C_YELLOW
.drawkey:
    call draw_text
    ret

section .data
str_hp:    db "HP", 0
str_ammo:  db "AMMO", 0
str_lives: db "LIVES", 0
str_key:   db "KEY", 0
dn_val:    dd 0
dn_x:      dd 0
dn_y:      dd 0
dn_col:    dd 0
dn_buf:    times 5 db 0
