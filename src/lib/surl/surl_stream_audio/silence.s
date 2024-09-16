; --------------------------------------
silence:
ssil:   lda     ser_status
        and     #HAS_BYTE
        beq     silence
dsil:   ldx     ser_data
start_duty:
        JMP_NEXT_6

.ifdef IIGS
slow_iigs:
        ; Slow down IIgs
        jsr     _get_iigs_speed
        sta     prevspd
        lda     #SPEED_SLOW
        jmp    _set_iigs_speed
.endif
