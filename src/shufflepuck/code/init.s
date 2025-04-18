        .import   _backup_barsnd
        .import   _load_barsnd
        .import   _init_text_before_decompress
        .import   _backup_table_high
        .import   _load_table_high
        .import   _backup_bar_code
        .import   _bar_load_scores
        .import   _load_bar_code
        .import   _cache_working
        .import   _backup_bar_high
        .import   _load_bar_high
        .import   _cputs
        .import   _init_text, _init_hgr
        .import   _clrscr
        .import   _cgetc
        .import   _exit
        .import   _load_lc
        .import   _load_lowcode
        .import   _init_mouse
        .import   _calibrate_hz
        .import   _strerror, ___errno

        .constructor init

.segment "ONCE"

load_splc_str:    .byte "LOADING LC CODE..."       ,$0D,$0A,$00
load_table_str:   .byte "LOADING TABLE IMAGE..."   ,$0D,$0A,$00
load_barsnd_str:  .byte "LOADING INTRO SOUND..."   ,$0D,$0A,$00
load_bar_str:     .byte "LOADING BAR IMAGE..."     ,$0D,$0A,$00
load_bar_code_str:.byte "LOADING BAR CODE..."      ,$0D,$0A,$00
load_err_str:     .byte "COULD NOT LOAD FILE: "    ,$00
no_mouse_str:     .byte "NO MOUSE DETECTED. PRESS A KEY TO EXIT."  ,$00

mouse_init_err:   .byte 0

.proc init
        ; Calibrate to get Hz
        jsr     _calibrate_hz

        ; Init mouse first thing (it flickers HGR on IIplus)
        ; But disable IRQs as cc65's handler is in LOWCODE,
        ; which is not yet loaded. We will enable IRQs once we
        ; reach main, so that the mouse interrupts don't start to
        ; poke stuff in BSS still containing ONCE code.
        php
        sei

        jsr     _init_mouse
        bcc     :+                ; Do we have a mouse?
        inc     mouse_init_err    ; No, we'll crash once we have lowcode
                                  ; (for cputs)

        ; Load LOWCODE (and splash screen)
:       jsr     _load_lowcode
        bcc     :+
        brk                       ; I can't be bothered to COUT manually

:       ; We can now enable IRQs, the handler is loaded
        plp

        jsr     _clrscr

        lda     mouse_init_err
        beq     :+

        lda     #<no_mouse_str
        ldx     #>no_mouse_str
        jmp     print_error

:       lda     #1
        jsr     _init_hgr

        lda     #<load_splc_str
        ldx     #>load_splc_str
        jsr     _cputs
        jsr     _load_lc
        bcc     :+
        brk

:       ; Preload assets at $4000 and back them up
        ; while we have the splash screen at $2000
        lda     #<load_bar_str
        ldx     #>load_bar_str
        jsr     _cputs
        jsr     _load_bar_high
        bcc     :+
        jmp     print_load_error

:       jsr     _backup_bar_high

        ; Don't bother preloading the rest if /RAM is unavailable
        lda     _cache_working
        beq     out

        lda     #<load_bar_code_str
        ldx     #>load_bar_code_str
        jsr     _cputs
        jsr     _load_bar_code
        bcc     :+
        jmp     print_load_error

:       jsr     _bar_load_scores
        jsr     _backup_bar_code

        lda     #<load_table_str
        ldx     #>load_table_str
        jsr     _cputs
        jsr     _load_table_high
        bcc     :+
        jmp     print_load_error

:       jsr     _backup_table_high

        lda     #<load_barsnd_str
        ldx     #>load_barsnd_str
        jsr     _cputs

        jsr     _clrscr

        ; Remove splashscreen as the intro sound
        ; overwrites it
        ; Ask data loader do it as late as possible
        lda     #1
        sta     _init_text_before_decompress
        jsr     _load_barsnd
        bcc     :+
        jmp     print_load_error

:       jsr     _backup_barsnd
        lda     #0
        sta     _init_text_before_decompress
out:    rts
.endproc

.proc print_load_error
        lda     ___errno
        ldx     #$00
        jsr     _strerror
        ; Fallthrough
.endproc
.proc print_error
        jsr     _cputs
        jsr     _init_text
        jsr     _cgetc
        jmp     _exit
.endproc
