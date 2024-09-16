duty_cycle18:
        ____SPKR_DUTY____4      ; 4     Toggle speaker

        lda     #INV_SPC        ; 6    Set VU meter
v18a:   sta     txt_level       ; 10

s18:    lda     ser_status      ; 14    Check serial
        and     #HAS_BYTE       ; 16
        beq     :+              ; 18/19
d18:    ldx     ser_data        ; 22    Load serial

        ____SPKR_DUTY____4      ; 26    Toggle speaker
        lda     #SPC            ; 28    Unset VU meter
        WASTE_7                 ; 35

v18b:   sta     txt_level       ; 39
        JMP_NEXT_6              ; 45
:
        WASTE_3                 ;    22
        ____SPKR_DUTY____4      ;    26 Toggle speaker
        WASTE_9                 ;    35
        KBD_LOAD_7              ;    42
        jmp     duty_cycle18    ;    45
