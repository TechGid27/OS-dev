; tiles.inc - tile map rendering + camera + tile queries
; camera in whole pixels; map_data / MAP_W / MAP_H from level.inc (generated)

section .text

; ---------------------------------------------------------------------
; camera_follow: eax = target center x (px), ebx = target center y (px)
camera_follow:
    sub  eax, SCREEN_W / 2
    jns  .cx1
    xor  eax, eax
.cx1:
    mov  ecx, MAP_W * 16 - SCREEN_W
    cmp  eax, ecx
    jle  .cx2
    mov  eax, ecx
.cx2:
    mov  [cam_x], eax

    sub  ebx, SCREEN_H / 2
    jns  .cy1
    xor  ebx, ebx
.cy1:
    mov  ecx, MAP_H * 16 - SCREEN_H
    cmp  ebx, ecx
    jle  .cy2
    mov  ebx, ecx
.cy2:
    mov  [cam_y], ebx
    ret

; ---------------------------------------------------------------------
; tile_at: eax = tx, ebx = ty -> eax = tile id
; OOB left/right/bottom = ground(1), OOB top = empty(0)
tile_at:
    cmp  eax, 0
    jl   .oob_solid
    cmp  eax, MAP_W
    jge  .oob_solid
    cmp  ebx, 0
    jl   .oob_empty
    cmp  ebx, MAP_H
    jge  .oob_solid
    imul ebx, ebx, MAP_W
    add  eax, ebx
    movzx eax, byte [map_data + eax]
    ret
.oob_solid:
    mov  eax, T_GROUND
    ret
.oob_empty:
    xor  eax, eax
    ret

; ---------------------------------------------------------------------
; id_is_solid: eax = tile id -> eax = 1 solid / 0 passable
id_is_solid:
    cmp  eax, 1
    jb   .no
    cmp  eax, T_EXIT                ; 4 = exit, passable
    je   .no
    cmp  eax, T_GROUND_TOP          ; 1,2,3,5 solid; 6+ passable
    ja   .no
    mov  eax, 1
    ret
.no:
    xor  eax, eax
    ret

; ---------------------------------------------------------------------
draw_map:
    mov  eax, [cam_x]
    shr  eax, 4
    mov  [tm_tx0], eax
    mov  eax, [cam_x]
    add  eax, SCREEN_W - 1
    shr  eax, 4
    mov  [tm_tx1], eax
    mov  eax, [cam_y]
    shr  eax, 4
    mov  [tm_ty0], eax
    mov  eax, [cam_y]
    add  eax, SCREEN_H - 1
    shr  eax, 4
    mov  [tm_ty1], eax

    mov  eax, [tm_ty0]
    mov  [tm_ty], eax
.ty:
    mov  eax, [tm_tx0]
    mov  [tm_tx], eax
.tx:
    ; id = map_data[ty*MAP_W + tx]
    mov  eax, [tm_tx]
    mov  ebx, [tm_ty]
    call tile_at
    test eax, eax
    jz   .next
    mov  [tm_id], eax

    ; screen pos
    mov  eax, [tm_tx]
    shl  eax, 4
    sub  eax, [cam_x]
    mov  [tm_sx], eax
    mov  ebx, [tm_ty]
    shl  ebx, 4
    sub  ebx, [cam_y]

    ; draw tile art (16x16, no flip)
    mov  eax, [tm_id]
    mov  eax, [tile_art_ptrs + eax * 4]
    mov  edx, 0
    mov  esi, 16
    mov  edi, 16
    mov  ecx, ebx                   ; sy
    mov  ebx, [tm_sx]
    call draw_ascii

.next:
    inc  dword [tm_tx]
    mov  eax, [tm_tx]
    cmp  eax, [tm_tx1]
    jbe  .tx
    inc  dword [tm_ty]
    mov  eax, [tm_ty]
    cmp  eax, [tm_ty1]
    jbe  .ty
    ret

section .data
cam_x: dd 0
cam_y: dd 0
tm_tx0: dd 0
tm_tx1: dd 0
tm_ty0: dd 0
tm_ty1: dd 0
tm_tx:  dd 0
tm_ty:  dd 0
tm_id:  dd 0
tm_sx:  dd 0
