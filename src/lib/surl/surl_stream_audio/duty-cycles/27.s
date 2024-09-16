duty_cycle27:
        ____SPKR_DUTY____4      ; 4     Toggle speaker

        lda     #INV_SPC        ; 6    Set VU meter
v27a:   sta     txt_level       ; 10

        WASTE_3                 ; 13    Wait...

s27:    lda     ser_status      ; 17    Check serial
        and     #HAS_BYTE       ; 19
        beq     :+              ; 21/22
d27:    ldx     ser_data        ; 25    Load serial

        lda     #SPC            ; 27    Unset VU meter
        WASTE_4                 ; 31

        ____SPKR_DUTY____4      ; 35    Toggle speaker
v27b:   sta     txt_level       ; 39
        JMP_NEXT_6              ; 45
:
        WASTE_9                 ;    31
        ____SPKR_DUTY____4      ;    35 Toggle speaker
        KBD_LOAD_7              ;    42
        jmp     duty_cycle27    ;    45
