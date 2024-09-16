duty_cycle26:
        ____SPKR_DUTY____4      ; 4     Toggle speaker

        lda     #INV_SPC        ; 6    Set VU meter
v26a:   sta     txt_level       ; 10

        WASTE_2                 ; 12    Wait...

s26:    lda     ser_status      ; 16    Check serial
        and     #HAS_BYTE       ; 18
        beq     :+              ; 20/21
d26:    ldx     ser_data        ; 24    Load serial

        lda     #SPC            ; 26    Unset VU meter
v26b:   sta     txt_level       ; 30

        ____SPKR_DUTY____4      ; 34    Toggle speaker
        WASTE_5                 ; 39
        JMP_NEXT_6              ; 45
:
        KBD_LOAD_7              ;    28
        WASTE_2                 ;    30
        ____SPKR_DUTY____4      ;    34 Toggle speaker
        WASTE_8                 ;    42
        jmp     duty_cycle26    ;    45
