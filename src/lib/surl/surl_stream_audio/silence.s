; --------------------------------------
silence:
        jsr     _get_iigs_speed
        sta     prevspd
        lda     #SPEED_SLOW
        jsr    _set_iigs_speed
ssil:   lda     ser_status
        and     has_byte
        beq     silence
dsil:   ldx     ser_data
        STORE_TARGET_3
start_duty:
        JMP_NEXT_6
