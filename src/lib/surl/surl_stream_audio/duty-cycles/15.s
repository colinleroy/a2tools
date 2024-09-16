duty_cycle15:
        ____SPKR_DUTY____4      ; 4     Toggle speaker

        lda     #INV_SPC        ; 6    Set VU meter
v15a:   sta     txt_level       ; 10

s15:    lda     ser_status      ; 14    Check serial
        and     #HAS_BYTE       ; 16
        beq     :+              ; 18/19

        ____SPKR_DUTY____5 15   ; 23    Toggle speaker
d15:    ldx     ser_data        ; 27    Load serial

        lda     #SPC            ; 29    Unset VU meter
        WASTE_6                 ; 35

v15b:   sta     txt_level       ; 39
        JMP_NEXT_6              ; 45
:
        ____SPKR_DUTY____4      ;    23 Toggle speaker
        WASTE_12                ;    35
        KBD_LOAD_7              ;    42
        jmp     duty_cycle15    ;    45
