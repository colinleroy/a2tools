duty_cycle27:
        DEBUG_JMP   #'R'
        ____SPKR_DUTY____4      ; 4     Toggle speaker

        lda     #INV_SPC        ; 6    Set VU meter
v27a:   sta     txt_level       ; 10

s27:    lda     ser_status      ; 14    Check serial
        and     has_byte        ; 17
        beq     :+              ; 19/20

d27:    ldx     ser_data        ; 23    Load serial
        ldy     safe_jumps,x    ; 27
        sty     j27+2           ; 31
        ____SPKR_DUTY____4      ; 35    Toggle speaker
        lda     spc             ; 38
v27b:   sta     txt_level       ; 42
j27:    jmp     $FF00           ; 45

:
        WASTE_11                ;    31
        ____SPKR_DUTY____4      ;    35 Toggle speaker
        KBD_LOAD_7              ;    42
        jmp     duty_cycle27    ;    45
