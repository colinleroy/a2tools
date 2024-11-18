break_out:
.ifdef IIGS
        lda     prevspd
        jsr     _set_iigs_speed
.endif
        plp

        jsr     _simple_serial_flush
        lda     #$2F            ; SURL_CLIENT_READY
        jsr     _serial_putc_direct
        lda     cancelled
        ldx     #0
        rts
