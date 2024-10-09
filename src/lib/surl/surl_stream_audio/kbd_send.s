; --------------------------------------
kbd_send:
        and     #$7F            ; Clear high bit
        bit     KBDSTRB         ; Clear keyboard strobe
        cmp     #' '            ; Space (pause?)
        beq     pause
        cmp     #'n'            ; n (end playback, next in list)
        beq     @next
        cmp     #'N'            ; N (end playback, next in list)
        beq     @next
        cmp     #$1B            ; Escape (end playback, cancel list)?
        bne     :+
        sta     cancelled
@next:
        lda     #$1B
:       jmp     _serial_putc_direct
pause:
        jsr     _serial_putc_direct
:       lda     KBD
        bpl     :-
        and     #$7F            ; Clear high bit
        bit     KBDSTRB         ; Clear keyboard strobe
        jmp     _serial_putc_direct
