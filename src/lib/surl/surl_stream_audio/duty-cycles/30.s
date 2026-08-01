duty_cycle30:
        DEBUG_JMP   #'U'
        ____SPKR_DUTY____4      ; 4     Toggle speaker

s30:    lda     ser_status      ; 8     Check serial
        and     has_byte        ; 11
        beq     :+              ; 13/14
d30:    ldx     ser_data        ; 17    Load serial
        ldy     safe_jumps,x    ; 21
        sty     j30+2           ; 25
        WASTE_9                 ; 34
        ____SPKR_DUTY____4      ; 38    Toggle speaker
        WASTE_4                 ; 42
j30:    jmp     $FF00           ; 45

:
        lda     #INV_SPC        ;    16 Set VU meter
v30a:   sta     txt_level       ;    20
        KBD_LOAD_7              ;    27
        WASTE_5                 ;    32
        lda     #SPC            ;    34
        ____SPKR_DUTY____4      ;    38 Toggle speaker
v30b:   sta     txt_level       ;    42 Unset VU meter
        jmp     duty_cycle30    ;    45
