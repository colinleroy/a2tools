duty_cycle17:
        ____SPKR_DUTY____4      ; 4     Toggle speaker

        lda     #INV_SPC        ; 6    Set VU meter
v17a:   sta     txt_level       ; 10

s17:    lda     ser_status      ; 14    Check serial
        and     has_byte        ; 17
        beq     :+              ; 19/20

        ldy     #SPC            ; 21    Unset VU meter
        ____SPKR_DUTY____4      ; 25    Toggle speaker
d17:    ldx     ser_data        ; 29    Load serial
        STORE_TARGET_3          ; 32
        WASTE_3                 ; 35

v17b:   sty     txt_level       ; 39
        JMP_NEXT_6              ; 45
:
        ____SPKR_DUTY____5 17   ;    25 Toggle speaker
        WASTE_10                ;    35
        KBD_LOAD_7              ;    42
        jmp     duty_cycle17    ;    45
