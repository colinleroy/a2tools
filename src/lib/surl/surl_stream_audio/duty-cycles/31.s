duty_cycle31:
        ____SPKR_DUTY____4      ; 4     Toggle speaker

        lda     #INV_SPC        ; 6     Set VU meter
v31a:   sta     txt_level       ; 10

        ldy     #SPC            ; 12    Unset VU meter

s31:    lda     ser_status      ; 16    Check serial
        and     has_byte        ; 19
        beq     :+              ; 21/22
d31:    ldx     ser_data        ; 25    Load serial

v31b:   sty     txt_level       ; 29
        STORE_TARGET_3          ; 32
        WASTE_3                 ; 35
        ____SPKR_DUTY____4      ; 39    Toggle speaker

        JMP_NEXT_6              ; 45
:
        WASTE_6                 ;    28
        KBD_LOAD_7              ;    35
        ____SPKR_DUTY____4      ;    39 Toggle speaker
        WASTE_3                 ;    42
        jmp     duty_cycle31    ;    45
