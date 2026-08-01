duty_cycle18:
        DEBUG_JMP   #'I'
        ____SPKR_DUTY____4      ; 4     Toggle speaker

        lda     #INV_SPC        ; 6    Set VU meter
v18a:   sta     txt_level       ; 10

s18:    lda     ser_status      ; 14    Check serial
        and     has_byte        ; 17
        bne     c18             ; 19/20

        WASTE_3                 ; 22
        ____SPKR_DUTY____4      ; 26 Toggle speaker
        WASTE_9                 ; 35
        KBD_LOAD_7              ; 42
        jmp     duty_cycle18    ; 45

c18:
        lda     #SPC            ; 22
        ____SPKR_DUTY____4      ; 26    Toggle speaker

d18:    ldx     ser_data        ; 30    Load serial
        ldy     safe_jumps,x    ; 34
        sty     j18+2           ; 38

v18b:   sta     txt_level       ; 42
j18:    jmp     $FF00           ; 45
