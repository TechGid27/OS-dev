; physics.inc - gravity + AABB vs tilemap collision (8.8 fixed point)
;
; phys_step: ebx -> x, ecx -> y, edx -> vx, esi -> vy   (all 8.8 dd)
;            [phys_w] / [phys_h] = body size in px
; returns:   eax = 1 on ground (hit floor this step), else 0

section .text

phys_step:
    mov  [p_x], ebx
    mov  [p_y], ecx
    mov  [p_vx], edx
    mov  [p_vy], esi

    ; ---- gravity ----
    mov  eax, [esi]
    add  eax, GRAVITY
    cmp  eax, MAX_FALL
    jle  .gv
    mov  eax, MAX_FALL
.gv:
    mov  [esi], eax

    ; ---- tile rows spanned by body (from current y) ----
    mov  eax, [ecx]
    sar  eax, 12
    mov  [p_ty0], eax
    mov  eax, [ecx]
    mov  ebx, [phys_h]
    dec  ebx
    shl  ebx, 8
    add  eax, ebx
    sar  eax, 12
    mov  [p_ty1], eax

    ; ================= X axis =================
    mov  eax, [p_vx]
    mov  eax, [eax]                 ; vx
    test eax, eax
    jz   .yaxis
    mov  ebx, [p_x]
    mov  ecx, [ebx]                 ; x
    add  ecx, eax                   ; newX
    mov  [p_nx], ecx
    test eax, eax
    js   .xleft

    ; moving right: probe right edge column
    mov  eax, ecx
    mov  ebx, [phys_w]
    shl  ebx, 8
    add  eax, ebx
    dec  eax
    sar  eax, 12
    call probe_xcol
    test eax, eax
    jz   .xmove
    ; snap left of that tile column
    mov  eax, [p_nx]
    mov  ebx, [phys_w]
    shl  ebx, 8
    add  eax, ebx
    dec  eax
    sar  eax, 12
    shl  eax, 12
    sub  eax, ebx
    mov  ebx, [p_x]
    mov  [ebx], eax
    mov  ebx, [p_vx]
    mov  dword [ebx], 0
    jmp  .yaxis
.xmove:
    mov  ebx, [p_x]
    mov  [ebx], ecx
    jmp  .yaxis

.xleft:
    mov  eax, ecx
    sar  eax, 12
    call probe_xcol
    test eax, eax
    jz   .xmove2
    mov  eax, [p_nx]
    sar  eax, 12
    inc  eax
    shl  eax, 12                    ; snap x = (tx+1) * 4096
    mov  ebx, [p_x]
    mov  [ebx], eax
    mov  ebx, [p_vx]
    mov  dword [ebx], 0
    jmp  .yaxis
.xmove2:
    mov  ebx, [p_x]
    mov  [ebx], ecx

    ; ================= Y axis =================
.yaxis:
    ; tile columns spanned by body (from x after X move)
    mov  eax, [p_x]
    mov  eax, [eax]
    sar  eax, 12
    mov  [p_tx0], eax
    mov  eax, [p_x]
    mov  eax, [eax]
    mov  ebx, [phys_w]
    shl  ebx, 8
    add  eax, ebx
    dec  eax
    sar  eax, 12
    mov  [p_tx1], eax

    mov  eax, [p_vy]
    mov  eax, [eax]                 ; vy
    mov  [p_vyt], eax
    mov  ebx, [p_y]
    mov  ecx, [ebx]                 ; y
    add  ecx, eax
    mov  [p_ny], ecx
    test eax, eax
    js   .yup

    ; falling: probe bottom edge row
    mov  eax, ecx
    mov  ebx, [phys_h]
    shl  ebx, 8
    add  eax, ebx
    dec  eax
    sar  eax, 12
    call probe_yrow
    test eax, eax
    jz   .yfall
    ; land
    mov  eax, [p_ny]
    mov  ebx, [phys_h]
    shl  ebx, 8
    add  eax, ebx
    dec  eax
    sar  eax, 12
    shl  eax, 12
    sub  eax, ebx
    mov  ebx, [p_y]
    mov  [ebx], eax
    mov  ebx, [p_vy]
    mov  dword [ebx], 0
    mov  eax, 1
    ret
.yfall:
    mov  ebx, [p_y]
    mov  [ebx], ecx
    xor  eax, eax
    ret

.yup:
    ; rising: probe top edge row
    mov  eax, ecx
    sar  eax, 12
    call probe_yrow
    test eax, eax
    jz   .yfall
    ; head bump: snap below tile
    mov  eax, [p_ny]
    sar  eax, 12
    inc  eax
    shl  eax, 12
    mov  ebx, [p_y]
    mov  [ebx], eax
    mov  ebx, [p_vy]
    mov  dword [ebx], 0
    xor  eax, eax
    ret

; ---- helpers (called from phys_step; share p_* temps) ----
; probe_xcol: eax = tx -> eax = 1 if solid at (tx,ty0) or (tx,ty1)
probe_xcol:
    mov  [p_t], eax
    mov  ebx, [p_ty0]
    call tile_at
    call id_is_solid
    test eax, eax
    jnz  .yes
    mov  eax, [p_t]
    mov  ebx, [p_ty1]
    call tile_at
    call id_is_solid
    test eax, eax
    jnz  .yes
    xor  eax, eax
    ret
.yes:
    mov  eax, 1
    ret

; probe_yrow: eax = ty -> eax = 1 if solid at (tx0,ty) or (tx1,ty)
probe_yrow:
    mov  [p_t], eax
    mov  eax, [p_tx0]
    mov  ebx, [p_t]
    call tile_at
    call id_is_solid
    test eax, eax
    jnz  .yes
    mov  eax, [p_tx1]
    mov  ebx, [p_t]
    call tile_at
    call id_is_solid
    test eax, eax
    jnz  .yes
    xor  eax, eax
    ret
.yes:
    mov  eax, 1
    ret

section .data
p_x:   dd 0
p_y:   dd 0
p_vx:  dd 0
p_vy:  dd 0
p_nx:  dd 0
p_ny:  dd 0
p_vyt: dd 0
p_ty0: dd 0
p_ty1: dd 0
p_tx0: dd 0
p_tx1: dd 0
p_t:   dd 0
phys_w: dd 16
phys_h: dd 16
