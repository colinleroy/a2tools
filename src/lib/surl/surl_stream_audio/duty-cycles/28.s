duty_cycle28:
        ____SPKR_DUTY____4      ; 4     Toggle speaker

        lda     #INV_SPC        ; 6    Set VU meter
v28a:   sta     txt_level       ; 10

        WASTE_4                 ; 14    Wait...

s28:    lda     ser_status      ; 18    Check serial
        and     #HAS_BYTE       ; 20
        beq     :+              ; 22/23
d28:    ldx     ser_data        ; 26    Load serial

        lda     #SPC            ; 28    Unset VU meter
v28b:   sta     txt_level       ; 32

        ____SPKR_DUTY____4      ; 36    Toggle speaker
        WASTE_3                 ; 39
        JMP_NEXT_6              ; 45
:
        KBD_LOAD_7              ;    30
        WASTE_2                 ;    32
        ____SPKR_DUTY____4      ;    36 Toggle speaker
        WASTE_6                 ;    42
        jmp     duty_cycle28    ;    45
