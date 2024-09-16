duty_cycle29:
        ____SPKR_DUTY____4      ; 4     Toggle speaker

        lda     #INV_SPC        ; 6    Set VU meter
v29a:   sta     txt_level       ; 10

        WASTE_5                 ; 15    Wait...

s29:    lda     ser_status      ; 19    Check serial
        and     #HAS_BYTE       ; 21
        beq     :+              ; 23/24
d29:    ldx     ser_data        ; 27    Load serial

        lda     #SPC            ; 29    Unset VU meter
v29b:   sta     txt_level       ; 33

        ____SPKR_DUTY____4      ; 37    Toggle speaker
        WASTE_2                 ; 39
        JMP_NEXT_6              ; 45
:
        KBD_LOAD_7              ;    31
        WASTE_2                 ;    33
        ____SPKR_DUTY____4      ;    37 Toggle speaker
        WASTE_5                 ;    42
        jmp     duty_cycle29    ;    45
