duty_cycle16:
        ____SPKR_DUTY____4      ; 4     Toggle speaker

        lda     #INV_SPC        ; 6    Set VU meter
v16a:   sta     txt_level       ; 10

s16:    lda     ser_status      ; 14    Check serial
        and     #HAS_BYTE       ; 16
        beq     :+              ; 18/19

        WASTE_2                 ; 20
        ____SPKR_DUTY____4      ; 24    Toggle speaker
d16:    ldx     ser_data        ; 28    Load serial

        lda     #SPC            ; 30    Unset VU meter
        WASTE_5                 ; 35

v16b:   sta     txt_level       ; 39
        JMP_NEXT_6              ; 45
:
        ____SPKR_DUTY____5 16   ;    24 Toggle speaker
        WASTE_11                ;    35
        KBD_LOAD_7              ;    42
        jmp     duty_cycle16    ;    45