duty_cycle7:
        DEBUG_JMP   #'7'
        ____SPKR_DUTY____4      ; 4     Toggle speaker

        lda     #INV_SPC        ; 6    Set VU meter
v7a:    sta     txt_level       ; 10
        ____SPKR_DUTY____5 7    ; 15    Toggle speaker
        
s7:     lda     ser_status      ; 19    Check serial
        and     has_byte        ; 22

        beq     :+              ; 24/25

d7:     ldx     ser_data        ; 28    Load serial

        lda     #SPC            ; 30    Unset VU meter
        ldy     safe_jumps,x    ; 34
        sty     j7+2            ; 38
v7b:    sta     txt_level       ; 42
j7:     jmp     $FF00           ; 45

:
        WASTE_10                ;    35
        KBD_LOAD_7              ;    42
        jmp     duty_cycle7     ;    45
