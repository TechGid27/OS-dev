section .text
; interrupts.inc - IDT (256 gates), PIC remap, common stubs

; ---------------------------------------------------------------------
idt_build:
    mov  edi, idt
    mov  ecx, 256
    mov  eax, irq_default
.fb:
    call gate_write
    add  edi, 8
    loop .fb

    mov  edi, idt + 32 * 8        ; IRQ0 -> timer
    mov  eax, irq0_handler
    call gate_write
    mov  edi, idt + 33 * 8        ; IRQ1 -> keyboard
    mov  eax, irq1_handler
    call gate_write

    lidt [idt_desc]
    ret

; gate_write: edi = dest (8 bytes), eax = handler offset
gate_write:
    mov  [edi], ax
    mov  word [edi + 2], 0x08     ; code selector
    mov  byte [edi + 4], 0
    mov  byte [edi + 5], 0x8E     ; present, ring0, 32-bit interrupt gate
    shr  eax, 16
    mov  [edi + 6], ax
    ret

; ---------------------------------------------------------------------
; remap PIC: IRQ0-7 -> int 32-39, IRQ8-15 -> 40-47; unmask IRQ0+IRQ1 only
int_init:
    mov  al, 0x11
    out  0x20, al
    out  0xA0, al
    mov  al, 0x20                 ; master vector offset 32
    out  0x21, al
    mov  al, 0x28                 ; slave vector offset 40
    out  0xA1, al
    mov  al, 0x04                 ; master: slave on IRQ2
    out  0x21, al
    mov  al, 0x02                 ; slave: cascade identity 2
    out  0xA1, al
    mov  al, 0x01                 ; 8086 mode
    out  0x21, al
    mov  al, 0xFC                 ; unmask IRQ0 (timer), IRQ1 (keyboard)
    out  0x21, al
    mov  al, 0xFF                 ; mask all slave IRQs
    out  0xA1, al
    sti
    ret

; ---------------------------------------------------------------------
irq_default:
    pushad
    mov  al, 0x20
    out  0x20, al
    popad
    iret

; ---------------------------------------------------------------------
section .data
idt:
    times 256 * 8 db 0
idt_desc:
    dw  256 * 8 - 1
    dd  idt
