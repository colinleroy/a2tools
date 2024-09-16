duty_cycle31:
        ____SPKR_DUTY____4      ; 4     Toggle speaker

        lda     #INV_SPC        ; 6     Set VU meter
v31a:   sta     txt_level       ; 10

        WASTE_7                 ; 17    Wait...

s31:    lda     ser_status      ; 21    Check serial
        and     #HAS_BYTE       ; 23
        beq     :+              ; 25/26
d31:    ldx     ser_data        ; 29    Load serial

        lda     #SPC            ; 31    Unset VU meter
v31b:   sta     txt_level       ; 35

        ____SPKR_DUTY____4      ; 39    Toggle speaker

        JMP_NEXT_6              ; 45
:
        KBD_LOAD_7              ;    33
        WASTE_2                 ;    35
        ____SPKR_DUTY____4      ;    39 Toggle speaker
        WASTE_3                 ;    42
        jmp     duty_cycle31    ;    45
