duty_cycle23:
        DEBUG_JMP   #'N'
        ____SPKR_DUTY____4      ; 4     Toggle speaker

        lda     #INV_SPC        ; 6    Set VU meter
v23a:   sta     txt_level       ; 10

s23:    lda     ser_status      ; 14    Check serial
        and     has_byte        ; 17
        beq     :+              ; 19/20

d23:    ldx     ser_data        ; 23    Load serial
        ldy     safe_jumps,x    ; 27
        ____SPKR_DUTY____4      ; 31    Toggle speaker
        lda     spc             ; 34    Unset VU meter
        sty     j23+2           ; 38
v23b:   sta     txt_level       ; 42
j23:    jmp     $FF00           ; 45

:
        WASTE_7                 ;    27
        ____SPKR_DUTY____4      ;    31 Toggle speaker
        WASTE_4                 ;    35
        KBD_LOAD_7              ;    42
        jmp     duty_cycle23    ;    45
