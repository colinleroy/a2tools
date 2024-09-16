duty_cycle13:
        ____SPKR_DUTY____4      ; 4     Toggle speaker

        lda     #INV_SPC        ; 6    Set VU meter
v13a:   sta     txt_level       ; 10

s13:    lda     ser_status      ; 14    Check serial
        and     #HAS_BYTE       ; 16
        ____SPKR_DUTY____5 13   ; 21    Toggle speaker

        beq     :+              ; 23/24

d13:    ldx     ser_data        ; 27    Load serial

        lda     #SPC            ; 29    Unset VU meter
        WASTE_6                 ; 35

v13b:   sta     txt_level       ; 39
        JMP_NEXT_6              ; 45
:
        WASTE_11                ;    35
        KBD_LOAD_7              ;    42
        jmp     duty_cycle13    ;    45
