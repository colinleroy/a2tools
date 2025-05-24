duty_cycle9:
        ____SPKR_DUTY____4      ; 4     Toggle speaker

        lda     #INV_SPC        ; 6    Set VU meter
v9a:    sta     txt_level       ; 10
        WASTE_3                 ; 13
        ____SPKR_DUTY____4      ; 17    Toggle speaker
        
s9:     lda     ser_status      ; 21    Check serial
        and     has_byte        ; 24

        beq     :+              ; 26/27

d9:     ldx     ser_data        ; 30    Load serial

        lda     #SPC            ; 32    Unset VU meter
        STORE_TARGET_3          ; 35

v9b:    sta     txt_level       ; 39
        JMP_NEXT_6              ; 45
:
        WASTE_8                 ;    35
        KBD_LOAD_7              ;    42
        jmp     duty_cycle9     ;    45
