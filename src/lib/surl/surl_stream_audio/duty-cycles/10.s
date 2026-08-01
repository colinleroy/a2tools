duty_cycle10:
        DEBUG_JMP   #'A'
        ____SPKR_DUTY____4      ; 4     Toggle speaker

        lda     #INV_SPC        ; 6    Set VU meter
v10a:   sta     txt_level       ; 10

s10:    lda     ser_status      ; 14    Check serial
        ____SPKR_DUTY____4      ; 18    Toggle speaker
        and     has_byte        ; 21

        bne     d10             ; 23/24

        WASTE_12                ; 35
        KBD_LOAD_7              ; 42
        jmp     duty_cycle10    ; 45

d10:    ldx     ser_data        ; 28    Load serial
        lda     #SPC            ; 30    Unset VU meter
        ldy     safe_jumps,x    ; 34
        sty     j10+2           ; 38

v10b:   sta     txt_level       ; 42
j10:    jmp     $FF00           ; 45
