duty_cycle21:
        ____SPKR_DUTY____4      ; 4     Toggle speaker

        lda     #INV_SPC        ; 6    Set VU meter
v21a:   sta     txt_level       ; 10

s21:    lda     ser_status      ; 14    Check serial
        and     has_byte        ; 17
        beq     :+              ; 19/20
d21:    ldx     ser_data        ; 23    Load serial

        lda     #SPC            ; 25    Unset VU meter
        ____SPKR_DUTY____4      ; 29    Toggle speaker
        STORE_TARGET_3          ; 32
        WASTE_3                 ; 35

v21b:   sta     txt_level       ; 39
        JMP_NEXT_6              ; 45
:
        WASTE_5                 ;    25
        ____SPKR_DUTY____4      ;    29 Toggle speaker
        WASTE_6                 ;    35
        KBD_LOAD_7              ;    42
        jmp     duty_cycle21    ;    45
