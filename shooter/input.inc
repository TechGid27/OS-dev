section .text
; input.inc - keyboard IRQ1, scancode -> key_state table
;
; key_state[scancode] = 1 held, 0 released (7-bit index, E0 prefix ignored)
; scancode KEY_* defines live in game.asm header

; ---------------------------------------------------------------------
irq1_handler:
    pushad
    inc  dword [key_evt_count]
    in   al, 0x60
    movzx ebx, al
    and  ebx, 0x7F                ; strip break bit
    test al, 0x80
    jnz  .up
    mov  byte [key_state + ebx], 1
    jmp  .eoi
.up:
    mov  byte [key_state + ebx], 0
.eoi:
    mov  al, 0x20
    out  0x20, al
    popad
    iret

; key_pressed: al = scancode -> eax = 1/0
key_pressed:
    movzx eax, al
    movzx eax, byte [key_state + eax]
    ret

section .data
key_state: times 128 db 0
key_evt_count: dd 0
