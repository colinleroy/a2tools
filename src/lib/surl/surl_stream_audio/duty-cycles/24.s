duty_cycle24:
        ____SPKR_DUTY____4      ; 4     Toggle speaker

        lda     #INV_SPC        ; 6    Set VU meter
v24a:   sta     txt_level       ; 10

s24:    lda     ser_status      ; 14    Check serial
        and     #HAS_BYTE       ; 16
        beq     :+              ; 18/19
d24:    ldx     ser_data        ; 22    Load serial

        lda     #SPC            ; 24    Unset VU meter
        WASTE_4                 ; 28

        ____SPKR_DUTY____4      ; 32    Toggle speaker
        WASTE_3                 ; 35
v24b:   sta     txt_level       ; 39
        JMP_NEXT_6              ; 45
:
        WASTE_9                 ;    28
        ____SPKR_DUTY____4      ;    32 Toggle speaker
        WASTE_3                 ;    35
        KBD_LOAD_7              ;    42
        jmp     duty_cycle24    ;    45