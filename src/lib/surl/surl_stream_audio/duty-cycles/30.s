duty_cycle30:
        DEBUG_JMP   #'U'
        ____SPKR_DUTY____4      ; 4     Toggle speaker

s30:    lda     ser_status      ; 8     Check serial
        and     has_byte        ; 11
        bne     d30             ; 13/14 INVERTED!

        lda     #INV_SPC        ; 15 Set VU meter
v30a:   sta     txt_level       ; 19
        KBD_LOAD_7              ; 26
        lda     #SPC            ; 28
        WASTE_2                 ; 30
v30b:   sta     txt_level       ; 34 Unset VU meter
        ____SPKR_DUTY____4      ; 38 Toggle speaker
        WASTE_5                 ; 43
        jmp     duty_cycle30    ; 46

d30:    ldx     ser_data        ; 18    Load serial
        WASTE_12                ; 30
        STORE_TARGET_4          ; 34
        ____SPKR_DUTY____4      ; 38    Toggle speaker
        WASTE_2
        JMP_NEXT_6              ; 46
