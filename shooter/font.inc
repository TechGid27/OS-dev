; font.inc - 8x8 ASCII-art font rendering (glyph table lives in art.inc)
; entry: font_table = db char, 64 bytes rows (8x8 '#'=on), next entry; db 0 end

section .text

; ---------------------------------------------------------------------
; put_pixel: eax=x, ebx=y, edx=color (low byte) - clips to screen
put_pixel:
    test eax, eax
    js   .ret
    cmp  eax, SCREEN_W
    jge  .ret
    test ebx, ebx
    js   .ret
    cmp  ebx, SCREEN_H
    jge  .ret
    imul ebx, ebx, SCREEN_W
    add  eax, ebx
    mov  [FB + eax], dl
.ret:
    ret

; ---------------------------------------------------------------------
; draw_text: esi = string (0-terminated), ebx = x, ecx = y, edx = color
draw_text:
    mov  [dt_str], esi
    mov  [dt_x], ebx
    mov  [dt_y], ecx
    mov  [dt_color], edx
.ch:
    mov  esi, [dt_str]
    lodsb
    test al, al
    jz   .done
    inc  dword [dt_str]
    ; find glyph
    mov  esi, font_table
.scan:
    mov  bl, [esi]
    test bl, bl
    jz   .ch_next                  ; unknown char: skip
    cmp  bl, al
    je   .found
    add  esi, 65
    jmp  .scan
.found:
    inc  esi                       ; esi -> 8 rows of 8 chars
    mov  dword [dt_row], 0
.row:
    mov  eax, [dt_row]
    lea  edi, [esi + eax * 8]
    mov  dword [dt_col], 0
.col:
    mov  eax, [dt_col]
    cmp  byte [edi + eax], '#'
    jne  .px
    mov  eax, [dt_col]
    imul eax, [dt_scale]
    add  eax, [dt_x]
    mov  ebx, [dt_row]
    imul ebx, [dt_scale]
    add  ebx, [dt_y]
    call put_pixel                 ; edx = color preserved by put_pixel
.px:
    inc  dword [dt_col]
    cmp  dword [dt_col], 8
    jb   .col
    inc  dword [dt_row]
    cmp  dword [dt_row], 8
    jb   .row
.ch_next:
    mov  eax, 8
    imul eax, [dt_scale]
    add  [dt_x], eax
    jmp  .ch
.done:
    ret

; draw_text_2x: same args as draw_text, double size
draw_text_2x:
    mov  dword [dt_scale], 2
    call draw_text
    mov  dword [dt_scale], 1
    ret

section .data
dt_scale: dd 1
dt_str:   dd 0
dt_x:     dd 0
dt_y:     dd 0
dt_color: dd 0
dt_row:   dd 0
dt_col:   dd 0
