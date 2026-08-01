duty_cycle13:
        DEBUG_JMP   #'D'
        ____SPKR_DUTY____4      ; 4     Toggle speaker

        lda     #INV_SPC        ; 6    Set VU meter
v13a:   sta     txt_level       ; 10

s13:    lda     ser_status      ; 14    Check serial
        and     has_byte        ; 17
        ____SPKR_DUTY____4      ; 21    Toggle speaker

        bne     d13             ; 23/24

        WASTE_12                ; 35
        KBD_LOAD_7              ; 42
        jmp     duty_cycle13    ; 45

d13:    ldx     ser_data        ; 28    Load serial

        lda     #SPC            ; 30    Unset VU meter
        ldy     safe_jumps,x    ; 34
        sty     j13+2           ; 38

v13b:   sta     txt_level       ; 42
j13:    jmp     $FF00           ; 45
