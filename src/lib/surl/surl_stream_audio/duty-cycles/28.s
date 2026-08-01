duty_cycle28:
        DEBUG_JMP   #'S'
        ____SPKR_DUTY____4      ; 4     Toggle speaker

        lda     #INV_SPC        ; 6    Set VU meter
v28a:   sta     txt_level       ; 10

s28:    lda     ser_status      ; 14    Check serial
        and     has_byte        ; 17
        bne     d28              ; 19/20

        KBD_LOAD_7              ;    26
        WASTE_6                 ;    32
        ____SPKR_DUTY____4      ;    36 Toggle speaker
        WASTE_6                 ;    42
        jmp     duty_cycle28    ;    45

d28:    ldx     ser_data        ; 24    Load serial
        ldy     safe_jumps,x    ; 28
        sty     j28+2           ; 32
        ____SPKR_DUTY____4      ; 36    Toggle speaker
        lda     #SPC            ; 38    Unset VU meter
v28b:   sta     txt_level       ; 42
j28:    jmp     $FF00           ; 45
