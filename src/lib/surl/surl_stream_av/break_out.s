break_out:
        jsr     _clrscr
        jsr     _text_mono40
        lda     #$01
        ldx     #$00
        jsr     _sleep

        lda     #$02            ; Disable second port
vcmd2:  sta     $98FF

        plp                     ; Reenable all interrupts
        jsr     _simple_serial_flush
        lda     #$2F            ; SURL_CLIENT_READY
        jsr     _serial_putc_direct

        lda     cancelled
        ldx     #0
        rts
