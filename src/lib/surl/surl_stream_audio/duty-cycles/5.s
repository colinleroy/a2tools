duty_cycle5:
        ____SPKR_DUTY____4      ; 4     Toggle speaker

        lda     #INV_SPC        ; 6    Set VU meter
        WASTE_3                 ; 9
        ____SPKR_DUTY____4      ; 13    Toggle speaker
v5a:    sta     txt_level       ; 17
        
s5:     lda     ser_status      ; 21    Check serial
        and     has_byte        ; 24

        beq     :+              ; 26/27

d5:     ldx     ser_data        ; 30    Load serial

        lda     #SPC            ; 32    Unset VU meter
        STORE_TARGET_3          ; 35

v5b:    sta     txt_level       ; 39
        JMP_NEXT_6              ; 45
:
        WASTE_8                 ;    35
        KBD_LOAD_7              ;    42
        jmp     duty_cycle5     ;    45
