duty_cycle6:
        ____SPKR_DUTY____4      ; 4     Toggle speaker

        lda     #INV_SPC        ; 6    Set VU meter
v6a:    sta     txt_level       ; 10
        ____SPKR_DUTY____4      ; 14    Toggle speaker
        
s6:     lda     ser_status      ; 18    Check serial
        and     #HAS_BYTE       ; 20

        beq     :+              ; 22/23

d6:     ldx     ser_data        ; 26    Load serial

        lda     #SPC            ; 28    Unset VU meter
        WASTE_7                 ; 35

v6b:    sta     txt_level       ; 39
        JMP_NEXT_6              ; 45
:
        WASTE_12                ;    35
        KBD_LOAD_7              ;    42
        jmp     duty_cycle6     ;    45
