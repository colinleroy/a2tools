duty_cycle6:
        DEBUG_JMP   #'6'
        ____SPKR_DUTY____4      ; 4     Toggle speaker

        lda     #INV_SPC        ; 6    Set VU meter
v6a:    sta     txt_level       ; 10
        ____SPKR_DUTY____4      ; 14    Toggle speaker
        
s6:     lda     ser_status      ; 18    Check serial
        and     has_byte        ; 21
        bne     d6              ; 23/24

        WASTE_12                ; 35
        KBD_LOAD_7              ; 42
        jmp     duty_cycle6     ; 45

d6:     ldx     ser_data        ; 28    Load serial

        lda     #SPC            ; 30    Unset VU meter
        ldy     safe_jumps,x    ; 34
        sty     j6+2            ; 38
v6b:    sta     txt_level       ; 42
j6:     jmp     $FF00           ; 45
