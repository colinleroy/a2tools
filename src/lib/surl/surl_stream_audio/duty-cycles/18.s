duty_cycle18:
        ____SPKR_DUTY____4      ; 4     Toggle speaker

        lda     #INV_SPC        ; 6    Set VU meter
v18a:   sta     txt_level       ; 10

s18:    lda     ser_status      ; 14    Check serial
        and     has_byte        ; 17
        beq     :+              ; 19/20
        WASTE_3                 ; 22
        ____SPKR_DUTY____4      ; 26    Toggle speaker

d18:    ldx     ser_data        ; 30    Load serial

        lda     #SPC            ; 32    Unset VU meter
        STORE_TARGET_3          ; 35

v18b:   sta     txt_level       ; 39
        JMP_NEXT_6              ; 45
:
        WASTE_2                 ;    22
        ____SPKR_DUTY____4      ;    26 Toggle speaker
        WASTE_9                 ;    35
        KBD_LOAD_7              ;    42
        jmp     duty_cycle18    ;    45
