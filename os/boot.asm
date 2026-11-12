; boot.asm - 512-byte boot sector (real mode)
; Loads stage2 from LBA 1..16 to 0000:8000 and jumps there with DL = drive.
;
; Build: nasm -f bin boot.asm -o boot.bin

BITS 16
ORG  0x7C00

%define STAGE2_SECTORS 16
%define SPT            18
%define HEADS          2

start:
    cli
    xor ax, ax
    mov ds, ax
    mov es, ax
    mov ss, ax
    mov sp, 0x7C00
    sti
    mov [drive], dl

    mov si, 1                       ; LBA counter
    mov di, 0x0800                  ; segment: LBA 1 -> phys 0x8000
.load:
    mov es, di
    xor bx, bx
    mov ax, si
    call read_one
    jc   fail
    inc si
    add di, 0x20                    ; 512 bytes = 0x20 paragraphs
    cmp si, STAGE2_SECTORS + 1
    jb  .load

    mov dl, [drive]
    jmp 0x0000:0x8000

; read_one: ax = LBA, es:bx = buffer. CF set on failure.
read_one:
    mov [lba], ax
    mov byte [tries], 3
.again:
    mov ax, [lba]
    xor dx, dx
    div word [spt]                  ; ax = LBA/18, dx = LBA%18
    mov cl, dl
    inc cl                          ; CL = sector (1-based)
    xor dx, dx
    div word [heads]                ; ax = cylinder, dx = head
    mov ch, al
    mov dh, dl
    mov dl, [drive]
    mov ax, 0x0201                  ; read 1 sector
    int 0x13
    jnc .ok
    xor ax, ax                      ; reset disk
    int 0x13
    dec byte [tries]
    jnz .again
    stc
    ret
.ok:
    clc
    ret

fail:
    mov ax, 0x0E45                  ; 'E'
    int 0x10
.halt:
    hlt
    jmp .halt

drive: db 0
lba:   dw 0
tries: db 0
spt:   dw SPT
heads: dw HEADS

times 510 - ($ - $$) db 0
dw 0xAA55
