duty_cycle9:
        DEBUG_JMP   #'9'
        ____SPKR_DUTY____4      ; 4     Toggle speaker

        lda     #INV_SPC        ; 6    Set VU meter
v9a:    sta     txt_level       ; 10
        ldy     spc             ; 13
        ____SPKR_DUTY____4      ; 17    Toggle speaker
        
s9:     lda     ser_status      ; 21    Check serial
        and     has_byte        ; 24

        beq     :+              ; 26/27

d9:     ldx     ser_data        ; 30    Load serial
v9b:    sty     txt_level       ; 42
        ldy     safe_jumps,x    ; 34
        sty     j9+2            ; 38
j9:     jmp     $FF00           ; 45

:
        WASTE_8                 ;    35
        KBD_LOAD_7              ;    42
        jmp     duty_cycle9     ;    45
