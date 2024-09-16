duty_cycle17:
        ____SPKR_DUTY____4      ; 4     Toggle speaker

        lda     #INV_SPC        ; 6    Set VU meter
v17a:   sta     txt_level       ; 10

s17:    lda     ser_status      ; 14    Check serial
        and     #HAS_BYTE       ; 16
        beq     :+              ; 18/19

        WASTE_3                 ; 21
        ____SPKR_DUTY____4      ; 25    Toggle speaker
d17:    ldx     ser_data        ; 29    Load serial

        lda     #SPC            ; 31    Unset VU meter
        WASTE_4                 ; 35

v17b:   sta     txt_level       ; 39
        JMP_NEXT_6              ; 45
:
        WASTE_2                 ;    21
        ____SPKR_DUTY____4      ;    25 Toggle speaker
        WASTE_10                ;    35
        KBD_LOAD_7              ;    42
        jmp     duty_cycle17    ;    45
