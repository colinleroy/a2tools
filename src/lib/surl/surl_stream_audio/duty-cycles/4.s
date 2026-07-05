duty_cycle4:
        DEBUG_JMP   #'4'
        ____SPKR_DUTY____4      ; 4     Toggle speaker

        lda     #INV_SPC        ; 6    Set VU meter
        ldy     #SPC            ; 8    Unset VU meter
        ____SPKR_DUTY____4      ; 12    Toggle speaker
v4a:    sta     txt_level       ; 16
        
s4:     lda     ser_status      ; 20    Check serial
        and     has_byte        ; 23

        beq     :+              ; 25/26

d4:     ldx     ser_data        ; 29    Load serial
        STORE_TARGET_3          ; 32
        WASTE_4                 ; 36

v4b:    sty     txt_level       ; 40
        JMP_NEXT_6              ; 46
:
        WASTE_10                ;    36
        KBD_LOAD_7              ;    43
        jmp     duty_cycle4     ;    46
