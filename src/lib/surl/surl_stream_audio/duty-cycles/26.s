duty_cycle26:
        DEBUG_JMP   #'Q'
        ____SPKR_DUTY____4      ; 4     Toggle speaker

        lda     #INV_SPC        ; 6    Set VU meter
v26a:   sta     txt_level       ; 10

s26:    lda     ser_status      ; 14    Check serial
        and     has_byte        ; 17
        bne     d26             ; 19/20 WARNING! Inverted!

        KBD_LOAD_7              ;    26
        WASTE_4                 ;    30
        ____SPKR_DUTY____4      ;    34 Toggle speaker
        WASTE_9                 ;    43
        jmp     duty_cycle26    ;    46

d26:    ldx     ser_data        ; 24    Load serial
        lda     #SPC            ; 26
v26b:   sta     txt_level       ; 30
        ____SPKR_DUTY____4      ; 34    Toggle speaker
        STORE_TARGET_3          ; 37
        WASTE_3                 ; 40
        JMP_NEXT_6              ; 46
