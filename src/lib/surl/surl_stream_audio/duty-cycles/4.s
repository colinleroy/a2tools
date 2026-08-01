duty_cycle4:
        DEBUG_JMP   #'4'
        ____SPKR_DUTY____4      ; 4     Toggle speaker

        lda     #INV_SPC        ; 6    Set VU meter
        ldy     #SPC            ; 8    Unset VU meter
        ____SPKR_DUTY____4      ; 12    Toggle speaker
v4a:    sta     txt_level       ; 16
        
s4:     lda     ser_status      ; 20    Check serial
        and     has_byte        ; 23

        bne     d4              ; 25/26

        WASTE_10                ; 35
        KBD_LOAD_7              ; 42
        jmp     duty_cycle4     ; 45

d4:     ldx     ser_data        ; 30    Load serial
v4b:    sty     txt_level       ; 34

        ldy     safe_jumps,x    ; 38
        sty     j4+2            ; 42
j4:     jmp     $FF00           ; 45
