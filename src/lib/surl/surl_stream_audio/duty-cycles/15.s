duty_cycle15:
        DEBUG_JMP   #'F'
        ____SPKR_DUTY____4      ; 4     Toggle speaker

        lda     #INV_SPC        ; 6    Set VU meter
v15a:   sta     txt_level       ; 10

s15:    lda     ser_status      ; 14    Check serial
        and     has_byte        ; 17
        ____SPKR_DUTY____4      ; 21    Toggle speaker
        bne     d15             ; 23/24

        WASTE_12                ; 35
        KBD_LOAD_7              ; 42
        jmp     duty_cycle15    ; 45

d15:    ldx     ser_data        ; 28    Load serial
        ldy     safe_jumps,x    ; 32
        sty     j15+2           ; 36
        lda     #SPC            ; 38    Unset VU meter
v15b:   sta     txt_level       ; 42
j15:    jmp     $FF00           ; 45
