duty_cycle8:
        ____SPKR_DUTY____4      ; 4     Toggle speaker

        lda     #INV_SPC        ; 6    Set VU meter
v8a:    sta     txt_level       ; 10
        WASTE_2                 ; 12
        ____SPKR_DUTY____4      ; 16    Toggle speaker
        
s8:     lda     ser_status      ; 20    Check serial
        and     #HAS_BYTE       ; 22

        beq     :+              ; 24/25

d8:     ldx     ser_data        ; 28    Load serial

        lda     #SPC            ; 30    Unset VU meter
        WASTE_5                 ; 35

v8b:    sta     txt_level       ; 39
        JMP_NEXT_6              ; 45
:
        WASTE_10                ;    35
        KBD_LOAD_7              ;    42
        jmp     duty_cycle8     ;    45
