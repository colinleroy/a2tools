duty_cycle29:
        DEBUG_JMP   #'T'
        ____SPKR_DUTY____4      ; 4     Toggle speaker

        lda     #INV_SPC        ; 6    Set VU meter
v29a:   sta     txt_level       ; 10

        ldy     #SPC            ; 12    Unset VU meter

s29:    lda     ser_status      ; 16    Check serial
        and     has_byte        ; 19
        bne     d29              ; 21/22 WARNING! inverted

        WASTE_5                 ; 26
        KBD_LOAD_7              ; 33
        ____SPKR_DUTY____4      ; 37 Toggle speaker
        WASTE_6                 ; 43
        jmp     duty_cycle29    ; 46

d29:    ldx     ser_data        ; 26    Load serial

v29b:   sty     txt_level       ; 30
        STORE_TARGET_3          ; 33
        ____SPKR_DUTY____4      ; 37    Toggle speaker
        WASTE_3                 ; 40
        JMP_NEXT_6              ; 46
