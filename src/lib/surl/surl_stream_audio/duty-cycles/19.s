duty_cycle19:
        DEBUG_JMP   #'J'
        ____SPKR_DUTY____4      ; 4     Toggle speaker

        lda     #INV_SPC        ; 6    Set VU meter
v19a:   sta     txt_level       ; 10

s19:    lda     ser_status      ; 14    Check serial
        and     has_byte        ; 17
        beq     :+              ; 19/20

d19:    ldx     ser_data        ; 23    Load serial
        ____SPKR_DUTY____4      ; 27    Toggle speaker
        lda     spc             ; 30    Unset VU meter
        ldy     safe_jumps,x    ; 34
        sty     j19+2           ; 38
v19b:   sta     txt_level       ; 42
j19:    jmp     $FF00           ; 45

:
        WASTE_3                 ;    23
        ____SPKR_DUTY____4      ;    27 Toggle speaker
        WASTE_8                 ;    35
        KBD_LOAD_7              ;    42
        jmp     duty_cycle19    ;    45
