duty_cycle11:
        DEBUG_JMP   #'B'
        ____SPKR_DUTY____4      ; 4     Toggle speaker

        lda     #INV_SPC        ; 6    Set VU meter
v11a:   sta     txt_level       ; 10

s11:    lda     ser_status      ; 14    Check serial
        ____SPKR_DUTY____5 11   ; 19    Toggle speaker
        and     has_byte        ; 22

        beq     :+              ; 24/25

d11:    ldx     ser_data        ; 28    Load serial

        lda     #SPC            ; 30    Unset VU meter
        ldy     safe_jumps,x    ; 34
        sty     j11+2           ; 38

v11b:   sta     txt_level       ; 42
j11:    jmp     $FF00           ; 45

:
        WASTE_10                ;    35
        KBD_LOAD_7              ;    42
        jmp     duty_cycle11    ;    45
