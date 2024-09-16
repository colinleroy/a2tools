duty_cycle25:
        ____SPKR_DUTY____4      ; 4     Toggle speaker

        lda     #INV_SPC        ; 6    Set VU meter
v25a:   sta     txt_level       ; 10

s25:    lda     ser_status      ; 14    Check serial
        and     #HAS_BYTE       ; 16
        beq     :+              ; 18/19
d25:    ldx     ser_data        ; 22    Load serial

        lda     #SPC            ; 24    Unset VU meter
        WASTE_5                 ; 29

        ____SPKR_DUTY____4      ; 33    Toggle speaker
        WASTE_2                 ; 35
v25b:   sta     txt_level       ; 39
        JMP_NEXT_6              ; 45
:
        WASTE_10                ;    29
        ____SPKR_DUTY____4      ;    33 Toggle speaker
        WASTE_2                 ;    35
        KBD_LOAD_7              ;    42
        jmp     duty_cycle25    ;    45
