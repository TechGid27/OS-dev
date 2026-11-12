section .text
; sound.inc - PC speaker SFX via PIT channel 2 (ports 43h/42h, gate 61h b0-1)
;
; main thread:  sfx_play / sfx_queue  (freq in eax, duration ticks in edx)
; IRQ0 context: sfx_tick advances timers / note queue
; single channel: short SFX override each other; jingles use the queue

%define SFX_QUEUE_LEN 16

; ---------------------------------------------------------------------
sfx_tick:
    cmp  dword [sfx_timer], 0
    je   .check_q
    dec  dword [sfx_timer]
    jnz  .done
    ; current note expired -> speaker off
    in   al, 0x61
    and  al, 0xFC
    out  0x61, al
.check_q:
    movzx eax, byte [sfx_q_len]
    test eax, eax
    jz   .done
    ; pop head of FIFO
    movzx ebx, byte [sfx_q_pos]
    movzx ecx, word [sfx_q_freq + ebx * 2]
    movzx edx, word [sfx_q_dur + ebx * 2]
    inc  bl
    and  bl, SFX_QUEUE_LEN - 1
    mov  [sfx_q_pos], bl
    dec  byte [sfx_q_len]
    mov  eax, ecx
    call sfx_set_tone
    mov  [sfx_timer], edx
.done:
    ret

; ---------------------------------------------------------------------
; sfx_set_tone: eax = Hz (0 = silence)
sfx_set_tone:
    test eax, eax
    jz   .off
    push ecx
    push edx
    mov  ecx, eax                  ; ecx = freq
    mov  eax, 1193182
    xor  edx, edx
    div  ecx                       ; eax = 1193182 / freq = divisor
    push eax
    mov  al, 0xB6                  ; ch2, lobyte/hibyte, mode 3
    out  0x43, al
    pop  eax
    out  0x42, al                  ; divisor low
    mov  al, ah
    out  0x42, al                  ; divisor high
    in   al, 0x61
    or   al, 3                     ; timer2 gate + speaker data
    out  0x61, al
    pop  edx
    pop  ecx
    ret
.off:
    in   al, 0x61
    and  al, 0xFC
    out  0x61, al
    ret

; ---------------------------------------------------------------------
; sfx_play: eax = Hz, edx = ticks (immediate, overrides current)
sfx_play:
    pushf
    cli
    mov  [sfx_timer], edx
    call sfx_set_tone
    popf
    ret

; sfx_queue: eax = Hz, edx = ticks (append; FIFO)
sfx_queue:
    pushf
    cli
    cmp  byte [sfx_q_len], SFX_QUEUE_LEN
    jae  .full
    movzx ebx, byte [sfx_q_pos]
    movzx ecx, byte [sfx_q_len]
    add  ebx, ecx
    and  bl, SFX_QUEUE_LEN - 1
    mov  [sfx_q_freq + ebx * 2], ax
    mov  [sfx_q_dur + ebx * 2], dx
    inc  byte [sfx_q_len]
.full:
    popf
    ret

; ---- SFX helpers (main thread) ----
sfx_shoot:
    mov  eax, 1000
    mov  edx, 3
    jmp  sfx_play

sfx_jump:
    mov  eax, 700
    mov  edx, 4
    jmp  sfx_play

sfx_hit:
    mov  eax, 150
    mov  edx, 6
    jmp  sfx_play

sfx_hurt:
    mov  eax, 220
    mov  edx, 8
    jmp  sfx_play

sfx_pickup:
    push eax
    push edx
    mov  eax, 880
    mov  edx, 3
    call sfx_queue
    mov  eax, 1320
    mov  edx, 6
    call sfx_queue
    pop  edx
    pop  eax
    ret

sfx_enemy_die:
    mov  eax, 400
    mov  edx, 4
    call sfx_queue
    mov  eax, 200
    mov  edx, 6
    jmp  sfx_queue

sfx_door:
    mov  eax, 300
    mov  edx, 5
    call sfx_queue
    mov  eax, 500
    mov  edx, 5
    jmp  sfx_queue

sfx_clear:                         ; level clear jingle (C E G C)
    push eax
    push edx
    mov  eax, 523
    mov  edx, 8
    call sfx_queue
    mov  eax, 659
    mov  edx, 8
    call sfx_queue
    mov  eax, 784
    mov  edx, 8
    call sfx_queue
    mov  eax, 1047
    mov  edx, 20
    call sfx_queue
    pop  edx
    pop  eax
    ret

sfx_gameover:                      ; descending (G Eb C)
    push eax
    push edx
    mov  eax, 392
    mov  edx, 12
    call sfx_queue
    mov  eax, 311
    mov  edx, 12
    call sfx_queue
    mov  eax, 262
    mov  edx, 24
    call sfx_queue
    pop  edx
    pop  eax
    ret

; ---------------------------------------------------------------------
section .data
sfx_timer: dd 0
sfx_q_len: db 0
sfx_q_pos: db 0
sfx_q_freq: times SFX_QUEUE_LEN dw 0
sfx_q_dur:  times SFX_QUEUE_LEN dw 0
