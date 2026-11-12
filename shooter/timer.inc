section .text
; timer.inc - PIT channel 0 at ~60 Hz, IRQ0 handler

%define TICK_HZ 60

timer_init:
    mov  al, 0x36                 ; ch0, lobyte/hibyte, square wave
    out  0x43, al
    mov  ax, 1193182 / TICK_HZ    ; divisor = 19886
    out  0x40, al                 ; low byte
    mov  al, ah
    out  0x40, al                 ; high byte
    ret

; ---------------------------------------------------------------------
irq0_handler:
    pushad
    inc  dword [tick_count]
    call sfx_tick
    mov  al, 0x20
    out  0x20, al                 ; EOI
    popad
    iret

section .data
tick_count: dd 0
