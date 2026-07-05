duty_cycle28:
        DEBUG_JMP   #'S'
        ____SPKR_DUTY____4      ; 4     Toggle speaker

        lda     #INV_SPC        ; 6    Set VU meter
v28a:   sta     txt_level       ; 10

        ldy     #SPC            ; 12    Unset VU meter

s28:    lda     ser_status      ; 16    Check serial
        and     has_byte        ; 19
        beq     :+              ; 21/22
d28:    ldx     ser_data        ; 25    Load serial

v28b:   sty     txt_level       ; 29
        STORE_TARGET_3          ; 32
        ____SPKR_DUTY____4      ; 36    Toggle speaker
        WASTE_4                 ; 40
        JMP_NEXT_6              ; 46
:
        KBD_LOAD_7              ;    29
        WASTE_3                 ;    32
        ____SPKR_DUTY____4      ;    36 Toggle speaker
        WASTE_7                 ;    43
        jmp     duty_cycle28    ;    46
