duty_cycle8:
        ____SPKR_DUTY____4      ; 4     Toggle speaker

        lda     #INV_SPC        ; 6    Set VU meter
v8a:    sta     txt_level       ; 10
        ldy     #SPC            ; 12    Unset VU meter
        ____SPKR_DUTY____4      ; 16    Toggle speaker
        
s8:     lda     ser_status      ; 20    Check serial
        and     has_byte        ; 23

        beq     :+              ; 25/26

d8:     ldx     ser_data        ; 29    Load serial
        STORE_TARGET_3          ; 32
        WASTE_3                 ; 35

v8b:    sty     txt_level       ; 39
        JMP_NEXT_6              ; 45
:
        WASTE_9                 ;    35
        KBD_LOAD_7              ;    42
        jmp     duty_cycle8     ;    45
