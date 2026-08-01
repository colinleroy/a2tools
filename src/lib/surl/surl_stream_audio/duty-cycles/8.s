duty_cycle8:
        DEBUG_JMP   #'8'
        ____SPKR_DUTY____4      ; 4     Toggle speaker

        lda     #INV_SPC        ; 6    Set VU meter
v8a:    sta     txt_level       ; 10
        ldy     #SPC            ; 12    Unset VU meter
        ____SPKR_DUTY____4      ; 16    Toggle speaker
        
s8:     lda     ser_status      ; 20    Check serial
        and     has_byte        ; 23

        bne     d8              ; 25/26
        WASTE_10                ; 35
        KBD_LOAD_7              ; 42
        jmp     duty_cycle8     ; 45

d8:     ldx     ser_data        ; 30    Load serial
v8b:    sty     txt_level       ; 34
        ldy     safe_jumps,x    ; 38
        sty     j8+2            ; 42
j8:     jmp     $FF00           ; 45
