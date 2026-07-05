duty_cycle22:
        DEBUG_JMP   #'M'
        ____SPKR_DUTY____4      ; 4     Toggle speaker

        lda     #INV_SPC        ; 6    Set VU meter
v22a:   sta     txt_level       ; 10

s22:    lda     ser_status      ; 14    Check serial
        and     has_byte        ; 17
        beq     :+              ; 19/20

d22:    ldx     ser_data        ; 23    Load serial
        WASTE_3                 ; 26
        ____SPKR_DUTY____4      ; 30    Toggle speaker

        lda     #SPC            ; 32    Unset VU meter
v22b:   sta     txt_level       ; 36
        STORE_TARGET_4          ; 40
        JMP_NEXT_6              ; 46
:
        WASTE_6                 ;    26
        ____SPKR_DUTY____4      ;    30 Toggle speaker
        WASTE_6                 ;    36
        KBD_LOAD_7              ;    43
        jmp     duty_cycle22    ;    46
