duty_cycle27:
        ____SPKR_DUTY____4      ; 4     Toggle speaker

        lda     #INV_SPC        ; 6    Set VU meter
v27a:   sta     txt_level       ; 10

        WASTE_3                 ; 13    Wait...

s27:    lda     ser_status      ; 17    Check serial
        and     has_byte        ; 20
        beq     :+              ; 22/23
d27:    ldx     ser_data        ; 26    Load serial

        lda     #SPC            ; 28    Unset VU meter
        STORE_TARGET_3          ; 31

        ____SPKR_DUTY____4      ; 35    Toggle speaker
v27b:   sta     txt_level       ; 39
        JMP_NEXT_6              ; 45
:
        WASTE_8                 ;    31
        ____SPKR_DUTY____4      ;    35 Toggle speaker
        KBD_LOAD_7              ;    42
        jmp     duty_cycle27    ;    45
