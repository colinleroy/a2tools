; --------------------------------------
kbd_send:
        and     #$7F            ; Clear high bit
        bit     KBDSTRB         ; Clear keyboard strobe
        cmp     #' '
        beq     pause
        cmp     #$1B            ; Escape ?
        bne     :+
        sta     cancelled
:       jmp     _serial_putc_direct
pause:
        jsr     _serial_putc_direct
:       lda     KBD
        bpl     :-
        and     #$7F            ; Clear high bit
        bit     KBDSTRB         ; Clear keyboard strobe
        jmp     _serial_putc_direct
