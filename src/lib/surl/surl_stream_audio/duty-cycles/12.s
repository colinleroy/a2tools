duty_cycle12:
        ____SPKR_DUTY____4      ; 4     Toggle speaker

        lda     #INV_SPC        ; 6    Set VU meter
v12a:   sta     txt_level       ; 10

s12:    lda     ser_status      ; 14    Check serial
        and     #HAS_BYTE       ; 16
        ____SPKR_DUTY____4      ; 20    Toggle speaker

        beq     :+              ; 22/23

d12:    ldx     ser_data        ; 26    Load serial

        lda     #SPC            ; 28    Unset VU meter
        WASTE_7                 ; 35

v12b:   sta     txt_level       ; 39
        JMP_NEXT_6              ; 45
:
        WASTE_12                ;    35
        KBD_LOAD_7              ;    42
        jmp     duty_cycle12    ;    45
