duty_cycle25:
        DEBUG_JMP   #'P'
        ____SPKR_DUTY____4      ; 4     Toggle speaker

s25:    lda     ser_status      ; 8    Check serial
        and     has_byte        ; 11
        bne     d25             ; 13/14

        lda     #INV_SPC        ; 15    Set VU meter
v25a:   sta     txt_level       ; 19
        KBD_LOAD_7              ; 26
        WASTE_3                 ; 29
        ____SPKR_DUTY____4      ; 33 Toggle speaker
        WASTE_3                 ; 36
        lda     #SPC            ; 38    Unset VU meter
v25b:   sta     txt_level       ; 42
        jmp     duty_cycle25    ; 45

d25:    ldx     ser_data        ; 18    Load serial
        ldy     safe_jumps,x    ; 22
        WASTE_7                 ; 29
        ____SPKR_DUTY____4      ; 33    Toggle speaker
        sty     j25+2           ; 37
        WASTE_5                 ; 42
j25:    jmp     $FF00           ; 45
