duty_cycle20:
        ____SPKR_DUTY____4      ; 4     Toggle speaker

        lda     #INV_SPC        ; 6    Set VU meter
v20a:   sta     txt_level       ; 10

s20:    lda     ser_status      ; 14    Check serial
        and     #HAS_BYTE       ; 16
        beq     :+              ; 18/19
d20:    ldx     ser_data        ; 22    Load serial

        WASTE_2                 ; 24
        ____SPKR_DUTY____4      ; 28    Toggle speaker
        lda     #SPC            ; 30    Unset VU meter
        WASTE_5                 ; 35

v20b:   sta     txt_level       ; 39
        JMP_NEXT_6              ; 45
:
        WASTE_5                 ;    24
        ____SPKR_DUTY____4      ;    28 Toggle speaker
        WASTE_7                 ;    35
        KBD_LOAD_7              ;    42
        jmp     duty_cycle20    ;    45
