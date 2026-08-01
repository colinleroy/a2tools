; --------------------------------------
silence:
        jsr     _get_iigs_speed
        sta     prevspd
        lda     #SPEED_SLOW
        jsr    _set_iigs_speed
ssil:   lda     ser_status
        and     has_byte
        beq     ssil
dsil:   ldy     ser_data
        sty     start_duty+2
start_duty:
        jmp     $FF00
