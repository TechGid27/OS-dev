; boot.asm - 512-byte boot sector
; - sets VGA mode 13h (BIOS, last BIOS call)
; - loads payload from LBA 1..N to phys 0x10000
; - enables A20, loads GDT, switches to 32-bit protected mode
; - jumps to payload at 0x10000
;
; Build: nasm -f bin -DPAYLOAD_SECTORS=<n> boot.asm -o boot.bin

BITS 16
ORG  0x7C00

%ifndef PAYLOAD_SECTORS
%define PAYLOAD_SECTORS 64
%endif

%define PAYLOAD_SEG 0x1000           ; phys 0x10000
%define SPT        18                ; sectors per track (1.44M floppy)
%define HEADS      2

start:
    cli
    xor ax, ax
    mov ds, ax
    mov es, ax
    mov ss, ax
    mov sp, 0x7C00
    sti
    mov [boot_drive], dl

    ; ---- VGA mode 13h: 320x200x256 ----
    mov ax, 0x0013
    int 0x10

    ; ---- load payload sectors to 0x10000+ ----
    mov word [load_lba], 1
.load_loop:
    mov si, [load_lba]

    ; LBA -> CHS : sector = LBA%18+1, head = (LBA/18)%2, cyl = LBA/36
    mov ax, si
    xor dx, dx
    mov cx, SPT
    div cx                         ; ax = LBA/18, dx = LBA%18
    mov cl, dl
    inc cl                         ; CL = sector
    xor dx, dx
    mov bx, HEADS
    div bx                         ; ax = cylinder, dx = head
    mov ch, al                     ; CH = cylinder
    mov dh, dl                     ; DH = head

    ; dest segment = PAYLOAD_SEG + (LBA-1)*0x20   (512 bytes per segment step)
    mov ax, si
    dec ax
    shl ax, 5
    add ax, PAYLOAD_SEG
    mov es, ax
    xor bx, bx

    mov di, 3                      ; retries
.read_try:
    mov ax, 0x0201                 ; AH=02 read, AL=1 sector
    mov dl, [boot_drive]
    int 0x13
    jnc .read_ok
    xor ax, ax                     ; AH=00 reset disk
    int 0x13
    dec di
    jnz .read_try
    jmp disk_error

.read_ok:
    inc word [load_lba]
    mov ax, [load_lba]
    cmp ax, PAYLOAD_SECTORS + 1
    jbe .load_loop

    ; ---- enable A20 (fast) ----
    in  al, 0x92
    or  al, 2
    and al, 0xFE
    out 0x92, al

    ; ---- GDT + protected mode ----
    cli
    lgdt [gdt_desc]
    mov eax, cr0
    or  eax, 1
    mov cr0, eax
    jmp 0x08:pm_entry               ; far jump -> flush pipeline, CS=0x08

disk_error:
    mov si, msg_err
.pr:
    lodsb
    test al, al
    jz   .halt
    mov  ah, 0x0E
    mov  bx, 7
    int  0x10
    jmp  .pr
.halt:
    cli
    hlt
    jmp  .halt

; ---------------------------------------------------------------
BITS 32
pm_entry:
    mov ax, 0x10
    mov ds, ax
    mov es, ax
    mov ss, ax
    mov esp, 0x90000
    mov eax, 0x10000
    jmp eax                         ; -> game.asm entry

; ---------------------------------------------------------------
; data (still inside boot sector)
BITS 16
align 8
gdt:
    dq 0x0000000000000000           ; 0x00 null
    dq 0x00CF9A000000FFFF           ; 0x08 code: base0 limit4G 32-bit
    dq 0x00CF92000000FFFF           ; 0x10 data: base0 limit4G rw
gdt_end:
gdt_desc:
    dw gdt_end - gdt - 1
    dd gdt

boot_drive:  db 0
load_lba:    dw 0
msg_err:     db "DISK ERR", 0

times 510 - ($ - $$) db 0
dw 0xAA55
