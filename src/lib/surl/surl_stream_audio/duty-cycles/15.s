duty_cycle15:
        ____SPKR_DUTY____4      ; 4     Toggle speaker

        lda     #INV_SPC        ; 6    Set VU meter
v15a:   sta     txt_level       ; 10

        ldy     #SPC            ; 12    Unset VU meter

s15:    lda     ser_status      ; 16    Check serial
        and     has_byte        ; 19
        ____SPKR_DUTY____4      ; 23    Toggle speaker
        beq     :+              ; 25/26

d15:    ldx     ser_data        ; 29    Load serial
        STORE_TARGET_3          ; 32
        WASTE_3                 ; 35

v15b:   sty     txt_level       ; 39
        JMP_NEXT_6              ; 45
:
        WASTE_9                 ;    35
        KBD_LOAD_7              ;    42
        jmp     duty_cycle15    ;    45
