duty_cycle12:
        DEBUG_JMP   #'C'
        ____SPKR_DUTY____4      ; 4     Toggle speaker

        lda     #INV_SPC        ; 6    Set VU meter
v12a:   sta     txt_level       ; 10
        ldy     #SPC            ; 12    Unset VU meter

s12:    lda     ser_status      ; 16    Check serial
        ____SPKR_DUTY____4      ; 20    Toggle speaker
        and     has_byte        ; 23
        bne     d12             ; 25/26

        WASTE_10                ; 35
        KBD_LOAD_7              ; 42
        jmp     duty_cycle12    ; 45

d12:    ldx     ser_data        ; 30    Load serial
v12b:   sty     txt_level       ; 34
        ldy     safe_jumps,x    ; 38
        sty     j12+2           ; 42
j12:    jmp     $FF00           ; 45
