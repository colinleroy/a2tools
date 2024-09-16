duty_cycle2:
        ____SPKR_DUTY____4      ; 4     Toggle speaker

        lda     #INV_SPC        ; 6    Set VU meter
        ____SPKR_DUTY____4      ; 10    Toggle speaker
v2a:    sta     txt_level       ; 14
        
s2:     lda     ser_status      ; 18    Check serial
        and     #HAS_BYTE       ; 20

        beq     :+              ; 22/23

d2:     ldx     ser_data        ; 26    Load serial

        lda     #SPC            ; 28    Unset VU meter
        WASTE_7                 ; 35

v2b:    sta     txt_level       ; 39
        JMP_NEXT_6              ; 45
:
        WASTE_12                ;    35
        KBD_LOAD_7              ;    42
        jmp     duty_cycle2     ;    45
