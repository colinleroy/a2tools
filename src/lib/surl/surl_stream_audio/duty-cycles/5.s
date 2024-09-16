duty_cycle5:
        ____SPKR_DUTY____4      ; 4     Toggle speaker

        lda     #INV_SPC        ; 6    Set VU meter
        WASTE_3                 ; 9
        ____SPKR_DUTY____4      ; 13    Toggle speaker
v5a:    sta     txt_level       ; 17
        
s5:     lda     ser_status      ; 21    Check serial
        and     #HAS_BYTE       ; 23

        beq     :+              ; 25/26

d5:     ldx     ser_data        ; 29    Load serial

        lda     #SPC            ; 31    Unset VU meter
        WASTE_4                 ; 35

v5b:    sta     txt_level       ; 39
        JMP_NEXT_6              ; 45
:
        WASTE_9                 ;    35
        KBD_LOAD_7              ;    42
        jmp     duty_cycle5     ;    45
