duty_cycle9:
        ____SPKR_DUTY____4      ; 4     Toggle speaker

        lda     #INV_SPC        ; 6    Set VU meter
v9a:    sta     txt_level       ; 10
        WASTE_3                 ; 13
        ____SPKR_DUTY____4      ; 17    Toggle speaker
        
s9:     lda     ser_status      ; 21    Check serial
        and     #HAS_BYTE       ; 23

        beq     :+              ; 25/26

d9:     ldx     ser_data        ; 29    Load serial

        lda     #SPC            ; 31    Unset VU meter
        WASTE_4                 ; 35

v9b:    sta     txt_level       ; 39
        JMP_NEXT_6              ; 45
:
        WASTE_9                 ;    35
        KBD_LOAD_7              ;    42
        jmp     duty_cycle9     ;    45
