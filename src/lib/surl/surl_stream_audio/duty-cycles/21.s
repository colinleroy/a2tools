duty_cycle21:
        ____SPKR_DUTY____4      ; 4     Toggle speaker

        lda     #INV_SPC        ; 6    Set VU meter
v21a:   sta     txt_level       ; 10

s21:    lda     ser_status      ; 14    Check serial
        and     #HAS_BYTE       ; 16
        beq     :+              ; 18/19
d21:    ldx     ser_data        ; 22    Load serial

        WASTE_3                 ; 25
        ____SPKR_DUTY____4      ; 29    Toggle speaker
        lda     #SPC            ; 31    Unset VU meter
        WASTE_4                 ; 35

v21b:   sta     txt_level       ; 39
        JMP_NEXT_6              ; 45
:
        WASTE_6                 ;    25
        ____SPKR_DUTY____4      ;    29 Toggle speaker
        WASTE_6                 ;    35
        KBD_LOAD_7              ;    42
        jmp     duty_cycle21    ;    45
