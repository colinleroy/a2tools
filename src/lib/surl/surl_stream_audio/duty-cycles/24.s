duty_cycle24:
        DEBUG_JMP   #'O'
        ____SPKR_DUTY____4      ; 4     Toggle speaker

        lda     #INV_SPC        ; 6    Set VU meter
v24a:   sta     txt_level       ; 10

s24:    lda     ser_status      ; 14    Check serial
        and     has_byte        ; 17
        bne     d24             ; 19/20

        WASTE_9                 ;    28
        ____SPKR_DUTY____4      ;    32 Toggle speaker
        WASTE_3                 ;    35
        KBD_LOAD_7              ;    42
        jmp     duty_cycle24    ;    45

d24:    ldx     ser_data        ; 24    Load serial
        ldy     safe_jumps,x    ; 28
        ____SPKR_DUTY____4      ; 32    Toggle speaker
        sty     j24+2           ; 36
        lda     #SPC            ; 38    Unset VU meter
v24b:   sta     txt_level       ; 42
j24:    jmp     $FF00           ; 45
