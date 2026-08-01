duty_cycle17:
        DEBUG_JMP   #'H'
        ____SPKR_DUTY____4      ; 4     Toggle speaker

s17:    lda     ser_status      ; 8    Check serial
        and     has_byte        ; 11
        beq     :+              ; 13/14

d17:    ldx     ser_data        ; 17    Load serial
        ldy     safe_jumps,x    ; 21
        ____SPKR_DUTY____4      ; 25    Toggle speaker
        WASTE_7                 ; 32
        sty     j17+2           ; 36
        WASTE_6                 ; 42
j17:    jmp     $FF00           ; 45

:
        lda     #INV_SPC        ; 16    Set VU meter
        WASTE_5                 ; 21
        ____SPKR_DUTY____4      ; 25 Toggle speaker
v17a:   sta     txt_level       ; 29
        WASTE_7                 ; 36
        lda     #SPC            ; 38    Set VU meter
v17b:   sta     txt_level       ; 42
        jmp     duty_cycle17    ; 45
