duty_cycle22:
        ____SPKR_DUTY____4      ; 4     Toggle speaker

        lda     #INV_SPC        ; 6    Set VU meter
v22a:   sta     txt_level       ; 10

s22:    lda     ser_status      ; 14    Check serial
        and     has_byte        ; 17
        beq     :+              ; 19/20

        lda     #SPC            ; 21    Unset VU meter
v22b:   sta     txt_level       ; 25

        ____SPKR_DUTY____5 22   ; 30    Toggle speaker

d22:    ldx     ser_data        ; 34    Load serial
        STORE_TARGET_3          ; 37
        WASTE_2                 ; 39
        JMP_NEXT_6              ; 45
:
        WASTE_6                 ;    26
        ____SPKR_DUTY____4      ;    30 Toggle speaker
        WASTE_5                 ;    35
        KBD_LOAD_7              ;    42
        jmp     duty_cycle22    ;    45
