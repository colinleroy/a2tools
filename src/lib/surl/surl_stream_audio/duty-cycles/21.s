duty_cycle21:
        DEBUG_JMP   #'L'
        ____SPKR_DUTY____4      ; 4     Toggle speaker

s21:    lda     ser_status      ; 8    Check serial
        and     has_byte        ; 11
        bne     d21             ; 13/14

        lda     #INV_SPC        ; 15    Set VU meter
v21a:   sta     txt_level       ; 19
        WASTE_6                 ; 25
        ____SPKR_DUTY____4      ; 29 Toggle speaker
        KBD_LOAD_7              ; 36
        lda     #SPC            ; 38    Unset VU meter
v21b:   sta     txt_level       ; 42
        jmp     duty_cycle21    ; 45

d21:    ldx     ser_data        ; 18    Load serial
        ldy     safe_jumps,x    ; 22
        WASTE_3                 ; 25
        ____SPKR_DUTY____4      ; 29    Toggle speaker
        sty     j21+2           ; 33
        WASTE_9                 ; 42
j21:    jmp     $FF00           ; 45
