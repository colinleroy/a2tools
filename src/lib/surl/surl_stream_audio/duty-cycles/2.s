duty_cycle2:
        DEBUG_JMP   #'2'
        ____SPKR_DUTY____4      ; 4     Toggle speaker

        lda     #INV_SPC        ; 6    Set VU meter
        ____SPKR_DUTY____4      ; 10    Toggle speaker
v2a:    sta     txt_level       ; 14
        
s2:     lda     ser_status      ; 18    Check serial
        and     has_byte        ; 21
        bne     d2              ; 23/24

        WASTE_12                ;    35
        KBD_LOAD_7              ;    42
        jmp     duty_cycle2     ;    45

d2:     ldx     ser_data        ; 28    Load serial

        lda     #SPC            ; 30    Unset VU meter
        ldy     safe_jumps,x    ; 34
        sty     j2+2            ; 38

v2b:    sta     txt_level       ; 42
j2:     jmp     $FF00           ; 45
