duty_cycle15:
        DEBUG_JMP   #'F'
        ____SPKR_DUTY____4      ; 4     Toggle speaker

        lda     #INV_SPC        ; 6    Set VU meter
v15a:   sta     txt_level       ; 10

        ldy     #SPC            ; 12    Unset VU meter

s15:    lda     ser_status      ; 16    Check serial
        and     has_byte        ; 19
        ____SPKR_DUTY____4      ; 23    Toggle speaker
        beq     :+              ; 25/26

d15:    ldx     ser_data        ; 29    Load serial
        STORE_TARGET_3          ; 32
        WASTE_4                 ; 36

v15b:   sty     txt_level       ; 40
        JMP_NEXT_6              ; 46
:
        WASTE_10                ;    36
        KBD_LOAD_7              ;    43
        jmp     duty_cycle15    ;    46
