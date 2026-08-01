duty_cycle5:
        DEBUG_JMP   #'5'
        ____SPKR_DUTY____4      ; 4     Toggle speaker

        lda     #INV_SPC        ; 6    Set VU meter
        ldy     spc             ; 9
        ____SPKR_DUTY____4      ; 13    Toggle speaker
v5a:    sta     txt_level       ; 17
        
s5:     lda     ser_status      ; 21    Check serial
        and     has_byte        ; 24

        beq     :+              ; 26/27

d5:     ldx     ser_data        ; 30    Load serial
v5b:    sty     txt_level       ; 34

        ldy     safe_jumps,x    ; 38
        sty     j5+2            ; 42
j5:     jmp     $FF00           ; 45

:
        WASTE_8                 ;    35
        KBD_LOAD_7              ;    42
        jmp     duty_cycle5     ;    45
