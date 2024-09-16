duty_cycle4:
        ____SPKR_DUTY____4      ; 4     Toggle speaker

        lda     #INV_SPC        ; 6    Set VU meter
        WASTE_2                 ; 8
        ____SPKR_DUTY____4      ; 12    Toggle speaker
v4a:    sta     txt_level       ; 16
        
s4:     lda     ser_status      ; 20    Check serial
        and     #HAS_BYTE       ; 22

        beq     :+              ; 24/25

d4:     ldx     ser_data        ; 28    Load serial

        lda     #SPC            ; 30    Unset VU meter
        WASTE_5                 ; 35

v4b:    sta     txt_level       ; 39
        JMP_NEXT_6              ; 45
:
        WASTE_10                 ;    35
        KBD_LOAD_7              ;    42
        jmp     duty_cycle4     ;    45
