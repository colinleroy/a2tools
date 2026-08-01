duty_cycle29:
        DEBUG_JMP   #'T'
        ____SPKR_DUTY____4      ; 4     Toggle speaker

s29:    lda     ser_status      ; 8    Check serial
        and     has_byte        ; 11
        bne     d29             ; 13/14

        lda     #INV_SPC        ; 15    Set VU meter
v29a:   sta     txt_level       ; 19
        KBD_LOAD_7              ; 26
        lda     spc             ; 29    Unset VU meter
v29b:   sta     txt_level       ; 33
        ____SPKR_DUTY____4      ; 37 Toggle speaker
        WASTE_5                 ; 42
        jmp     duty_cycle29    ; 45

d29:    ldx     ser_data        ; 18    Load serial
        ldy     safe_jumps,x    ; 22
        sty     j29+2           ; 26
        WASTE_7                 ; 33
        ____SPKR_DUTY____4      ; 37    Toggle speaker
        WASTE_5                 ; 42
j29:    jmp     $FF00           ; 45
