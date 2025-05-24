duty_cycle12:
        ____SPKR_DUTY____4      ; 4     Toggle speaker

        lda     #INV_SPC        ; 6    Set VU meter
v12a:   sta     txt_level       ; 10
        ldy     #SPC            ; 12    Unset VU meter

s12:    lda     ser_status      ; 16    Check serial
        ____SPKR_DUTY____4      ; 20    Toggle speaker
        and     has_byte        ; 23
        beq     :+              ; 25/26

d12:    ldx     ser_data        ; 29    Load serial
        STORE_TARGET_3          ; 32
        WASTE_3                 ; 35

v12b:   sty     txt_level       ; 39
        JMP_NEXT_6              ; 45
:
        WASTE_9                 ;    35
        KBD_LOAD_7              ;    42
        jmp     duty_cycle12    ;    45
