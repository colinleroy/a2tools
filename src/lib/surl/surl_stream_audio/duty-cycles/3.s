duty_cycle3:
        DEBUG_JMP   #'3'
        ____SPKR_DUTY____4      ; 4     Toggle speaker

        lda     #INV_SPC        ; 6    Set VU meter
        ____SPKR_DUTY____5 3    ; 11    Toggle speaker
v3a:    sta     txt_level       ; 15
        
s3:     lda     ser_status      ; 19    Check serial
        and     has_byte        ; 22

        beq     :+              ; 24/25

d3:     ldx     ser_data        ; 28    Load serial

        lda     #SPC            ; 30    Unset VU meter
        ldy     safe_jumps,x    ; 34
        sty     j3+2            ; 38

v3b:    sta     txt_level       ; 42
j3:     jmp     $FF00           ; 45

:
        WASTE_10                ;    35
        KBD_LOAD_7              ;    42
        jmp     duty_cycle3     ;    45
