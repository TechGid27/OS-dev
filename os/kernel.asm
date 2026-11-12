; kernel.asm - small OS kernel, 32-bit protected mode, VGA mode 13h.
; Loaded by stage2 to 0x60000 and entered at its first byte.
;
; File browser keys (same as the slide):
;   O  HELLO.TXT   text file
;   P  PIC.IMG     picture (320x200)
;   M  MUSIC.SPK   music through the PC speaker
;   V  CLIP.VID    video, shown as a small picture
;   G  DUKE.BIN    the shooter game (ESC inside the game returns here)
; In the viewers, B or ESC goes back to the file list.
;
; Files are read from the directory that stage2 left at 0x500.

BITS 32
ORG  0x60000

%include "consts.inc"

%define VGA          0xA0000
%define DIR          0x500
%define DIR_ENTRIES  16
%define KSTACK       0x7FFF0
%define GAME_ENTRY   0x10000

%define SC_ESC 0x01
%define SC_B   0x30

%define TXT_COLS 38                 ; 8-pixel cells, 8px margins
%define TXT_ROWS 10                 ; 16-pixel cells

; =====================================================================
kernel_entry:
    cli
    mov ax, 0x10
    mov ds, ax
    mov es, ax
    mov ss, ax
    mov esp, KSTACK
    call tone_off
    call idt_init
    call pic_init
    call pit_init
    call palette_load
    mov byte [kb_key], 0
    sti

main_menu:
    call draw_menu
.wait:
    hlt
    call poll_key
    test eax, eax
    jz   .wait
    mov  esi, menu_tab
    mov  ecx, menu_count
.find:
    cmp  eax, [esi]
    je   .hit
    add  esi, 20
    dec  ecx
    jnz  .find
    jmp  .wait
.hit:
    call dword [esi+16]
    jmp  main_menu

; =====================================================================
; input

poll_key:                           ; -> eax = make scancode or 0
    movzx eax, byte [kb_key]
    mov  byte [kb_key], 0
    ret

wait_back:                          ; returns on B or ESC
.w:
    hlt
    call poll_key
    cmp  eax, SC_B
    je   .r
    cmp  eax, SC_ESC
    jne  .w
.r:
    ret

; =====================================================================
; file list

draw_menu:
    mov  ebx, 0                     ; green window background, like the slide
    mov  ecx, 0
    mov  edx, 320
    mov  esi, 200
    mov  al, C_WINBG
    call fill_rect
    mov  esi, s_title
    mov  ebx, 40
    mov  ecx, 8
    mov  dl, C_WHITE
    call draw_string

    ; icon grid: 3 columns x 2 rows, icons 32x32 (2x), name under each
    mov  dword [mi], 0
.item:
    mov  eax, [mi]
    xor  edx, edx
    mov  ecx, 3
    div  ecx                        ; eax = row, edx = column
    imul edx, edx, 96
    add  edx, 16
    mov  [cell_x], edx
    imul eax, eax, 60
    add  eax, 28
    mov  [cell_y], eax

    mov  eax, [mi]
    imul eax, eax, 20
    add  eax, menu_tab
    mov  [mptr], eax

    mov  eax, [mi]                  ; icon
    shl  eax, 8
    add  eax, icons
    mov  esi, eax
    mov  ebx, [cell_x]
    add  ebx, 32
    mov  ecx, [cell_y]
    call draw_icon_x2

    mov  esi, [mptr]                ; name, centred under the icon
    mov  esi, [esi+8]
    call str_len                    ; ecx = length
    mov  eax, ecx
    shl  eax, 3
    mov  ebx, 96
    sub  ebx, eax
    shr  ebx, 1
    add  ebx, [cell_x]
    mov  esi, [mptr]
    mov  esi, [esi+8]
    mov  ecx, [cell_y]
    add  ecx, 36
    mov  dl, C_WHITE
    call draw_string

    inc  dword [mi]
    cmp  dword [mi], menu_count
    jb   .item

    ; help lines, two columns: O P / M V / G Q
    mov  dword [mi], 0
.help:
    mov  eax, [mi]
    mov  edx, eax
    and  edx, 1
    imul edx, edx, 160
    add  edx, 8
    mov  ebx, edx                   ; x
    shr  eax, 1
    imul eax, eax, 16
    add  eax, 148
    mov  ecx, eax                   ; y
    mov  eax, [mi]
    imul eax, eax, 20
    mov  esi, [menu_tab + eax + 12] ; help text
    mov  dl, C_BLACK
    call draw_string
    inc  dword [mi]
    cmp  dword [mi], menu_count
    jb   .help
    ret

; =====================================================================
; text viewer: HELLO.TXT

view_text:
    mov  esi, f_hello
    call find_file
    test eax, eax
    jz   .ret
    mov  [v_addr], eax
    mov  [v_size], ecx
    call clear_screen
    mov  esi, f_hello
    mov  ebx, 8
    mov  ecx, 4
    mov  dl, C_WHITE
    call draw_string

    mov  dword [tx_col], 0
    mov  dword [tx_row], 0
    mov  esi, [v_addr]
    mov  edi, esi
    add  edi, [v_size]
.loop:
    cmp  esi, edi
    jae  .done
    mov  al, [esi]
    inc  esi
    cmp  al, 13
    je   .loop
    cmp  al, 10
    jne  .notnl
    call tx_newline
    jmp  .loop
.notnl:
    cmp  dword [tx_col], TXT_COLS
    jb   .put
    call tx_newline
.put:
    call text_put
    inc  dword [tx_col]
    jmp  .loop
.done:
    mov  esi, s_back
    mov  ebx, 8
    mov  ecx, 188
    mov  dl, C_GRAY
    call draw_string
    call wait_back
.ret:
    ret

tx_newline:
    mov  dword [tx_col], 0
    inc  dword [tx_row]
    ret

; text_put: al = char at the cursor. Keeps esi and edi.
text_put:
    cmp  dword [tx_row], TXT_ROWS
    jae  .skip
    mov  ebx, [tx_col]
    shl  ebx, 3
    add  ebx, 8
    mov  ecx, [tx_row]
    shl  ecx, 4
    add  ecx, 24
    mov  dl, C_TEXT
    call draw_char
.skip:
    ret

; =====================================================================
; picture viewer: PIC.IMG (64000 bytes, 320x200 palette indices)

view_pic:
    mov  esi, f_pic
    call find_file
    test eax, eax
    jz   .ret
    mov  esi, eax
    mov  edi, VGA
    mov  ecx, 16000
    rep  movsd
    mov  ebx, 0                     ; bottom bar with the back hint
    mov  ecx, 184
    mov  edx, 320
    mov  esi, 16
    mov  al, C_BLACK
    call fill_rect
    mov  esi, s_back
    mov  ebx, 8
    mov  ecx, 184
    mov  dl, C_WHITE
    call draw_string
    call wait_back
.ret:
    ret

; =====================================================================
; music player: MUSIC.SPK = (dw divisor, dw ticks)*, end with divisor 0.
; divisor 0xFFFF is a rest. One tick is 10 ms (IRQ0 runs at 100 Hz).

view_music:
    mov  esi, f_music
    call find_file
    test eax, eax
    jz   .ret
    mov  [mus_ptr], eax
    call clear_screen
    mov  esi, f_music
    mov  ebx, 8
    mov  ecx, 4
    mov  dl, C_WHITE
    call draw_string
    mov  esi, s_music_hint
    mov  ebx, 8
    mov  ecx, 188
    mov  dl, C_GRAY
    call draw_string
.note:
    mov  esi, [mus_ptr]
    movzx eax, word [esi]           ; divisor
    movzx ebx, word [esi+2]         ; duration in ticks
    cmp  eax, 0
    je   .end
    add  dword [mus_ptr], 4
    mov  edx, [ticks]
    add  edx, ebx
    mov  [mus_target], edx
    cmp  eax, 0xFFFF
    je   .rest
    call tone_on
    jmp  .wait
.rest:
    call tone_off
.wait:
    hlt
    call poll_key
    cmp  eax, SC_B
    je   .stop
    cmp  eax, SC_ESC
    je   .stop
    mov  eax, [ticks]
    cmp  eax, [mus_target]
    jb   .wait
    jmp  .note
.end:
    call tone_off
    call wait_back
    ret
.stop:
    call tone_off
.ret:
    ret

; tone_on: eax = PIT divisor for channel 2 (speaker)
tone_on:
    push eax
    mov  al, 0xB6
    out  0x43, al
    pop  eax
    out  0x42, al
    mov  al, ah
    out  0x42, al
    in   al, 0x61
    or   al, 3
    out  0x61, al
    ret

tone_off:
    in   al, 0x61
    and  al, 0xFC
    out  0x61, al
    ret

; =====================================================================
; video viewer: CLIP.VID = dw frames, dw 0, then 80x50 frames (4000 bytes)
; Each frame is drawn at 2x, so 160x100 on screen.

view_video:
    mov  esi, f_clip
    call find_file
    test eax, eax
    jz   .ret
    mov  [vid_base], eax
    movzx eax, word [eax]
    mov  [vid_count], eax
    call clear_screen
    mov  esi, f_clip
    mov  ebx, 8
    mov  ecx, 4
    mov  dl, C_WHITE
    call draw_string
    mov  esi, s_back
    mov  ebx, 8
    mov  ecx, 188
    mov  dl, C_GRAY
    call draw_string
    mov  dword [vid_idx], 0
.frame:
    mov  eax, [vid_idx]
    imul eax, eax, 4000
    add  eax, [vid_base]
    add  eax, 8
    mov  esi, eax
    mov  edi, VGA + 40*320 + 80
    mov  edx, 50
.row:
    push edi
    mov  ecx, 80
.col:
    mov  al, [esi]
    inc  esi
    mov  [edi], al
    mov  [edi+1], al
    mov  [edi+320], al
    mov  [edi+321], al
    add  edi, 2
    dec  ecx
    jnz  .col
    pop  edi
    add  edi, 640
    dec  edx
    jnz  .row

    mov  eax, [ticks]               ; 30 ticks = 300 ms per frame
    add  eax, 30
    mov  [vid_target], eax
.wait:
    hlt
    call poll_key
    cmp  eax, SC_B
    je   .ret
    cmp  eax, SC_ESC
    je   .ret
    mov  eax, [ticks]
    cmp  eax, [vid_target]
    jb   .wait
    mov  eax, [vid_idx]
    inc  eax
    cmp  eax, [vid_count]
    jb   .set
    xor  eax, eax
.set:
    mov  [vid_idx], eax
    jmp  .frame
.ret:
    ret

; =====================================================================
; game: jump into DUKE.BIN (loaded at 0x10000). ESC in the game jumps back
; to kernel_entry at 0x60000.

launch_game:
    cli
    call tone_off
    jmp  GAME_ENTRY

; =====================================================================
; directory lookup

; find_file: esi = 12-byte padded name. -> eax = load address, ecx = size.
; eax = 0 when the file is not in the directory.
find_file:
    push esi
    push edi
    push ebx
    push edx
    mov  ebx, DIR
    mov  edx, DIR_ENTRIES
.entry:
    test edx, edx
    jz   .nf
    push esi
    mov  edi, ebx
    mov  ecx, 12
    repe cmpsb
    pop  esi
    je   .found
    add  ebx, 32
    dec  edx
    jmp  .entry
.found:
    mov  eax, [ebx+20]
    mov  ecx, [ebx+24]
    jmp  .out
.nf:
    xor  eax, eax
    xor  ecx, ecx
.out:
    pop  edx
    pop  ebx
    pop  edi
    pop  esi
    ret

; =====================================================================
; drawing

; fill_rect: ebx=x, ecx=y, edx=w, esi=h, al=colour. Clobbers esi.
fill_rect:
    push edi
    push ecx
    push edx
    push ebx
    imul ecx, ecx, 320
    add  ecx, ebx
    mov  edi, VGA
    add  edi, ecx
.row:
    push edi
    mov  ecx, edx
    rep  stosb
    pop  edi
    add  edi, 320
    dec  esi
    jnz  .row
    pop  ebx
    pop  edx
    pop  ecx
    pop  edi
    ret

clear_screen:
    mov  ebx, 0
    mov  ecx, 0
    mov  edx, 320
    mov  esi, 200
    mov  al, C_BG
    call fill_rect
    ret

; draw_char: al = char, ebx = x, ecx = y, dl = colour. Keeps esi and edi.
draw_char:
    push esi
    push edi
    movzx esi, al
    shl  esi, 4
    add  esi, font
    mov  edi, ecx
    imul edi, edi, 320
    add  edi, ebx
    add  edi, VGA
    mov  ecx, 16
.r:
    mov  ah, [esi]
    push ecx
    mov  ecx, 8
.c:
    shl  ah, 1
    jnc  .s
    mov  [edi], dl
.s:
    inc  edi
    dec  ecx
    jnz  .c
    pop  ecx
    add  edi, 312
    inc  esi
    dec  ecx
    jnz  .r
    pop  edi
    pop  esi
    ret

; draw_icon: esi = 16x16 icon (one palette index per pixel, 0xFF transparent),
; ebx = x, ecx = y. Clobbers esi, eax, edx.
draw_icon:
    push edi
    push ecx
    push ebx
    mov  edi, ecx
    imul edi, edi, 320
    add  edi, ebx
    add  edi, VGA
    mov  edx, 16
.ir:
    push edi
    mov  ecx, 16
.ic:
    lodsb
    cmp  al, 0xFF
    je   .skip
    mov  [edi], al
.skip:
    inc  edi
    dec  ecx
    jnz  .ic
    pop  edi
    add  edi, 320
    dec  edx
    jnz  .ir
    pop  ebx
    pop  ecx
    pop  edi
    ret

; draw_icon_x2: esi = 16x16 icon, ebx = x, ecx = y; every pixel becomes 2x2.
; Clobbers esi, eax, edx.
draw_icon_x2:
    push edi
    push ecx
    push ebx
    mov  edi, ecx
    imul edi, edi, 320
    add  edi, ebx
    add  edi, VGA
    mov  edx, 16
.xr:
    push edi
    mov  ecx, 16
.xc:
    lodsb
    cmp  al, 0xFF
    je   .xs
    mov  [edi], al
    mov  [edi+1], al
    mov  [edi+320], al
    mov  [edi+321], al
.xs:
    add  edi, 2
    dec  ecx
    jnz  .xc
    pop  edi
    add  edi, 640
    dec  edx
    jnz  .xr
    pop  ebx
    pop  ecx
    pop  edi
    ret

; str_len: esi = zero-terminated string -> ecx = length (esi preserved)
str_len:
    push esi
    xor  ecx, ecx
.l:
    cmp  byte [esi], 0
    je   .d
    inc  esi
    inc  ecx
    jmp  .l
.d:
    pop  esi
    ret

; draw_string: esi = zero-terminated text, ebx = x, ecx = y, dl = colour
draw_string:
.l:
    lodsb
    test al, al
    jz   .d
    push esi
    push ebx
    push ecx
    call draw_char
    pop  ecx
    pop  ebx
    pop  esi
    add  ebx, 8
    jmp  .l
.d:
    ret

; =====================================================================
; interrupts: IRQ0 = timer (ticks), IRQ1 = keyboard (kb_key, make codes)

irq0:
    push eax
    inc  dword [ticks]
    mov  al, 0x20
    out  0x20, al
    pop  eax
    iretd

irq1:
    push eax
    in   al, 0x60
    test al, 0x80                   ; ignore key releases
    jnz  .done
    mov  [kb_key], al
.done:
    mov  al, 0x20
    out  0x20, al
    pop  eax
    iretd

idt_init:
    mov  eax, 0x20
    mov  ebx, irq0
    call idt_set
    mov  eax, 0x21
    mov  ebx, irq1
    call idt_set
    lidt [idt_desc]
    ret

; idt_set: eax = vector, ebx = handler address
idt_set:
    lea  edi, [idt + eax*8]
    mov  [edi], bx
    mov  word [edi+2], 0x08
    mov  byte [edi+4], 0
    mov  byte [edi+5], 0x8E
    shr  ebx, 16
    mov  [edi+6], bx
    ret

pic_init:
    mov  al, 0x11
    out  0x20, al
    out  0xA0, al
    mov  al, 0x20
    out  0x21, al
    mov  al, 0x28
    out  0xA1, al
    mov  al, 0x04
    out  0x21, al
    mov  al, 0x02
    out  0xA1, al
    mov  al, 0x01
    out  0x21, al
    out  0xA1, al
    mov  al, 0xFC                   ; unmask IRQ0 and IRQ1
    out  0x21, al
    mov  al, 0xFF
    out  0xA1, al
    ret

pit_init:                           ; channel 0, 100 Hz
    mov  al, 0x36
    out  0x43, al
    mov  ax, 11932
    out  0x40, al
    mov  al, ah
    out  0x40, al
    ret

palette_load:
    mov  dx, 0x3C8
    xor  al, al
    out  dx, al
    inc  dx
    mov  esi, palette
    mov  ecx, 192
    rep  outsb
    ret

; =====================================================================
; quit: ACPI soft-off (QEMU/Bochs/VirtualBox ports). If the machine is
; still running afterwards, show a message and halt.

shutdown_os:
    cli
    call tone_off
    mov  ax, 0x2000                 ; SLP_EN with SLP_TYP 0 = S5
    mov  dx, 0x604                  ; PIIX4 / ICH9 PM1a control
    out  dx, ax
    mov  dx, 0xB004                 ; older QEMU PM1a control
    out  dx, ax
    mov  ax, 0x3400                 ; Bochs / older QEMU
    mov  dx, 0x4004
    out  dx, ax
    call clear_screen
    mov  esi, s_off
    mov  ebx, 40
    mov  ecx, 92
    mov  dl, C_WHITE
    call draw_string
.halt:
    hlt
    jmp  .halt

; =====================================================================
; data

ticks:      dd 0
kb_key:     db 0
mi:         dd 0
mptr:       dd 0
my:         dd 0
cell_x:     dd 0
cell_y:     dd 0
v_addr:     dd 0
v_size:     dd 0
tx_col:     dd 0
tx_row:     dd 0
mus_ptr:    dd 0
mus_target: dd 0
vid_base:   dd 0
vid_count:  dd 0
vid_idx:    dd 0
vid_target: dd 0

; menu entry (20 bytes): scancode, letter, file name, description, handler
menu_count equ 6
menu_tab:
    dd 0x18, 'O', f_hello, d_text,  view_text
    dd 0x19, 'P', f_pic,   d_pic,   view_pic
    dd 0x32, 'M', f_music, d_music, view_music
    dd 0x2F, 'V', f_clip,  d_clip,  view_video
    dd 0x22, 'G', f_duke,  d_game,  launch_game
    dd 0x10, 'Q', f_quit,  d_quit,  shutdown_os

; directory names, 12 bytes each (same layout as the directory)
f_hello: db "HELLO.TXT", 0, 0, 0
f_pic:   db "PIC.IMG", 0, 0, 0, 0, 0
f_music: db "MUSIC.SPK", 0, 0, 0
f_clip:  db "CLIP.VID", 0, 0, 0, 0
f_duke:  db "DUKE.BIN", 0, 0, 0, 0
f_quit:  db "QUIT", 0, 0, 0, 0, 0, 0, 0, 0

d_text:  db "O: open text file", 0
d_pic:   db "P: open picture", 0
d_music: db "M: play music", 0
d_clip:  db "V: play video", 0
d_game:  db "G: play game", 0
d_quit:  db "Q: quit", 0

s_title:      db "=== Inside: GIDEON & VINCE ===", 0
s_menu_hint:  db "PRESS THE KEY OF A FILE", 0
s_back:       db "B: BACK", 0
s_music_hint: db "PLAYING   B: BACK", 0
s_off:        db "IT IS SAFE TO TURN OFF THE COMPUTER", 0

align 16
palette: incbin "palette.bin"          ; 64 colours, 6-bit RGB
font:    incbin "font8x16.bin"        ; 256 glyphs, 8x16, bit 7 = left pixel
icons:   incbin "icons.bin"           ; 6 menu icons, 16x16 each

align 8
idt:      times 256 * 8 db 0
idt_desc: dw 256 * 8 - 1
          dd idt

%if ($ - $$) > 0x10000
%error "kernel does not fit in 64 KB (0x60000-0x6FFFF)"
%endif
