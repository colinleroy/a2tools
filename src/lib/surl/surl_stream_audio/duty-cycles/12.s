duty_cycle12:
        DEBUG_JMP   #'C'
        ____SPKR_DUTY____4      ; 4     Toggle speaker

        lda     #INV_SPC        ; 6    Set VU meter
v12a:   sta     txt_level       ; 10
        ldy     #SPC            ; 12    Unset VU meter

s12:    lda     ser_status      ; 16    Check serial
        ____SPKR_DUTY____4      ; 20    Toggle speaker
        and     has_byte        ; 23
        beq     :+              ; 25/26

d12:    ldx     ser_data        ; 29    Load serial
        STORE_TARGET_3          ; 32
        WASTE_4                 ; 36

v12b:   sty     txt_level       ; 40
        JMP_NEXT_6              ; 46
:
        WASTE_10                ;    36
        KBD_LOAD_7              ;    43
        jmp     duty_cycle12    ;    46
