duty_cycle14:
        DEBUG_JMP   #'E'
        ____SPKR_DUTY____4      ; 4     Toggle speaker

        lda     #INV_SPC        ; 6    Set VU meter
v14a:   sta     txt_level       ; 10

s14:    lda     ser_status      ; 14    Check serial
        and     has_byte        ; 17
        ____SPKR_DUTY____5 14   ; 22    Toggle speaker

        beq     :+              ; 24/25

d14:    ldx     ser_data        ; 28    Load serial

        lda     #SPC            ; 30    Unset VU meter
        ldy     safe_jumps,x    ; 34
        sty     j14+2           ; 38
v14b:   sta     txt_level       ; 42
j14:    jmp     $FF00           ; 45

:
        WASTE_10                ;    35
        KBD_LOAD_7              ;    42
        jmp     duty_cycle14    ;    45
