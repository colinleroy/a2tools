break_out:
        jsr     _clrscr
        jsr     _text_mono40
        lda     #$01
        ldx     #$00
        jsr     _sleep

        ; Close second port. Limitations of the driver requires
        ; us to "open" it, close it, and reopen the data port
        lda     _ser_params + SIMPLE_SERIAL_PARAMS::PRINTER_SLOT
        ldx     #SER_BAUD_115200
        jsr     _serial_open
        jsr     _serial_close

        jsr     reopen_main_serial

        plp                     ; Reenable all interrupts
        jsr     _simple_serial_flush
        lda     #$2F            ; SURL_CLIENT_READY
        jsr     _serial_putc_direct

        lda     cancelled
        ldx     #0
        rts
