duty_cycle16:
        DEBUG_JMP   #'G'
        ____SPKR_DUTY____4      ; 4     Toggle speaker

        lda     #INV_SPC        ; 6    Set VU meter
v16a:   sta     txt_level       ; 10

s16:    lda     ser_status      ; 14    Check serial
        and     has_byte        ; 17
        beq     :+              ; 19/20

        ____SPKR_DUTY____5 16   ; 24    Toggle speaker
d16:    ldx     ser_data        ; 28    Load serial

        lda     #SPC            ; 30    Unset VU meter
        ldy     safe_jumps,x    ; 34
        sty     j16+2           ; 38

v16b:   sta     txt_level       ; 42
j16:    jmp     $FF00           ; 45

:
        ____SPKR_DUTY____4      ;    24 Toggle speaker
        WASTE_11                ;    35
        KBD_LOAD_7              ;    42
        jmp     duty_cycle16    ;    45
