duty_cycle0:                    ; Max negative level, 8 cycles
        DEBUG_JMP   #'0'
        ____SPKR_DUTY____4      ; 4     Toggle speaker
        ____SPKR_DUTY____4      ; 8     Toggle speaker
        lda     #INV_SPC        ; 10    Set VU meter
v0a:    sta     txt_level       ; 14
        
s0:     lda     ser_status      ; 18    Check serial
        and     has_byte        ; 21

        bne     d0              ; 23/24

        WASTE_12                ; 35
        KBD_LOAD_7              ; 42
        jmp     duty_cycle0     ; 45

d0:     ldx     ser_data        ; 28    Load serial

        lda     #SPC            ; 30    Unset VU meter
        ldy     safe_jumps,x    ; 34
        sty     j0+2            ; 38
v0b:    sta     txt_level       ; 42
j0:     jmp     $FF00           ; 45
