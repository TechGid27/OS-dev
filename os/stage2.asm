; stage2.asm - real-mode loader, entered at 0000:8000 from boot.asm
; - sets VGA mode 13h (last BIOS call)
; - reads the file directory (LBA 17) to 0000:0500
; - loads every directory entry to its load address (phys)
; - enables A20, switches to 32-bit protected mode, jumps to the kernel
;
; Directory entry (32 bytes):
;   0  name[12]   ASCIIZ, zero padded
;   12 lba        dd
;   16 sectors    dd
;   20 load addr  dd (physical)
;   24 size       dd (bytes)
;   28 reserved
;
; Build: nasm -f bin stage2.asm -o stage2.bin  (must fit 16 sectors = 8192 bytes)

BITS 16
ORG  0x8000

%define DIR_BUF     0x0500
%define BOUNCE      0x7E00          ; 512-byte bounce buffer (segment 0)
%define DIR_LBA     17
%define DIR_ENTRIES 16
%define KERNEL_BASE 0x60000
%define KERNEL_STACK 0x7FFF0

stage2_start:
    cli
    xor ax, ax
    mov ds, ax
    mov es, ax
    mov ss, ax
    mov sp, 0x7C00
    sti
    mov [drive], dl

    mov ax, 0x0013                  ; VGA mode 13h: 320x200x256
    int 0x10

    mov eax, DIR_LBA
    mov bx, DIR_BUF
    call read_sector
    jc   fail

    mov si, DIR_BUF
    mov bp, DIR_ENTRIES
.entry:
    cmp byte [si], 0
    je  .loaded
    call load_entry
    jc  fail
    add si, 32
    dec bp
    jnz .entry

.loaded:
    in   al, 0x92                   ; fast A20
    or   al, 2
    and  al, 0xFE
    out  0x92, al

    cli
    lgdt [gdt_desc]
    mov eax, cr0
    or  eax, 1
    mov cr0, eax
    jmp 0x08:pm_entry               ; flush pipeline, CS = 0x08

fail:
    mov ax, 0x0E46                  ; 'F'
    int 0x10
.halt:
    hlt
    jmp .halt

; load_entry: si = directory entry. Preserves si and bp. CF on error.
load_entry:
    push si
    mov eax, [si+12]
    mov [cur_lba], eax
    mov eax, [si+20]
    mov [cur_addr], eax
    mov eax, [si+16]
    mov [cur_cnt], eax
.sector:
    cmp dword [cur_cnt], 0
    je  .ok
    mov eax, [cur_lba]
    mov bx, BOUNCE
    call read_sector
    jc  .fail
    mov eax, [cur_addr]             ; copy 512 bytes to phys cur_addr
    mov edi, eax
    and edi, 0xF
    shr eax, 4
    mov es, ax
    mov esi, BOUNCE
    mov ecx, 128
    rep movsd
    xor ax, ax
    mov es, ax
    add dword [cur_addr], 512
    inc dword [cur_lba]
    dec dword [cur_cnt]
    jmp .sector
.ok:
    pop si
    clc
    ret
.fail:
    pop si
    stc
    ret

; read_sector: eax = LBA (< 65536), es:bx = destination. CF on error.
read_sector:
    pushad
    mov [rs_lba], ax
    mov byte [rs_try], 3
.try:
    mov ax, [rs_lba]
    xor dx, dx
    mov cx, 18                      ; sectors per track
    div cx                          ; ax = LBA/18, dx = LBA%18
    mov cl, dl
    inc cl                          ; sector 1-based
    xor dx, dx
    div word [heads_w]              ; ax = cylinder, dx = head
    mov ch, al
    mov dh, dl
    mov dl, [drive]
    mov ax, 0x0201
    int 0x13
    jnc .ok
    xor ax, ax
    int 0x13
    dec byte [rs_try]
    jnz .try
    popad
    stc
    ret
.ok:
    popad
    clc
    ret

heads_w dw 2

drive:    db 0
cur_lba:  dd 0
cur_addr: dd 0
cur_cnt:  dd 0
rs_lba:   dw 0
rs_try:   db 0

align 8
gdt:
    dq 0x0000000000000000           ; 0x00 null
    dq 0x00CF9A000000FFFF           ; 0x08 code: flat 32-bit
    dq 0x00CF92000000FFFF           ; 0x10 data: flat rw
gdt_end:
gdt_desc:
    dw gdt_end - gdt - 1
    dd gdt

BITS 32
pm_entry:
    mov ax, 0x10
    mov ds, ax
    mov es, ax
    mov ss, ax
    mov esp, KERNEL_STACK
    jmp KERNEL_BASE

; keep the image exactly within 16 sectors (8192 bytes)
%if ($ - $$) > 8192
%error "stage2 is larger than 16 sectors"
%endif
