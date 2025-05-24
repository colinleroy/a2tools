duty_cycle20:
        ____SPKR_DUTY____4      ; 4     Toggle speaker

        lda     #INV_SPC        ; 6    Set VU meter
v20a:   sta     txt_level       ; 10

s20:    lda     ser_status      ; 14    Check serial
        and     has_byte        ; 17
        beq     :+              ; 19/20

        lda     #SPC            ; 21    Unset VU meter
        WASTE_3                 ; 24
        ____SPKR_DUTY____4      ; 28    Toggle speaker
d20:    ldx     ser_data        ; 32    Load serial
        STORE_TARGET_3          ; 35

v20b:   sta     txt_level       ; 39
        JMP_NEXT_6              ; 45
:
        WASTE_4                 ;    24
        ____SPKR_DUTY____4      ;    28 Toggle speaker
        WASTE_7                 ;    35
        KBD_LOAD_7              ;    42
        jmp     duty_cycle20    ;    45
