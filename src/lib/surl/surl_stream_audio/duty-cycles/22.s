duty_cycle22:
        DEBUG_JMP   #'M'
        ____SPKR_DUTY____4      ; 4     Toggle speaker

        lda     #INV_SPC        ; 6    Set VU meter
v22a:   sta     txt_level       ; 10

s22:    lda     ser_status      ; 14    Check serial
        and     has_byte        ; 17
        beq     :+              ; 19/20

d22:    ldx     ser_data        ; 23    Load serial
        lda     spc             ; 26
        ____SPKR_DUTY____4      ; 30    Toggle speaker
        ldy     safe_jumps,x    ; 34
        sty     j22+2           ; 38

v22b:   sta     txt_level       ; 42
j22:    jmp     $FF00           ; 45

:
        WASTE_6                 ;    26
        ____SPKR_DUTY____4      ;    30 Toggle speaker
        WASTE_5                 ;    35
        KBD_LOAD_7              ;    42
        jmp     duty_cycle22    ;    45
