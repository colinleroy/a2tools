duty_cycle22:
        ____SPKR_DUTY____4      ; 4     Toggle speaker

        lda     #INV_SPC        ; 6    Set VU meter
v22a:   sta     txt_level       ; 10

s22:    lda     ser_status      ; 14    Check serial
        and     #HAS_BYTE       ; 16
        beq     :+              ; 18/19
d22:    ldx     ser_data        ; 22    Load serial

        lda     #SPC            ; 24    Unset VU meter
        WASTE_2                 ; 26

        ____SPKR_DUTY____4      ; 30    Toggle speaker
        WASTE_5                 ; 35
v22b:   sta     txt_level       ; 39
        JMP_NEXT_6              ; 45
:
        WASTE_7                 ;    26
        ____SPKR_DUTY____4      ;    30 Toggle speaker
        WASTE_5                 ;    35
        KBD_LOAD_7              ;    42
        jmp     duty_cycle22    ;    45
