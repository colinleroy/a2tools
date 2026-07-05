duty_cycle27:
        DEBUG_JMP   #'R'
        ____SPKR_DUTY____4      ; 4     Toggle speaker

        lda     #INV_SPC        ; 6    Set VU meter
v27a:   sta     txt_level       ; 10

s27:    lda     ser_status      ; 14   Check serial
        and     has_byte        ; 17
        beq     :+              ; 19/20
d27:    ldx     ser_data        ; 23    Load serial

        lda     #SPC            ; 25    Unset VU meter
v27b:   sta     txt_level       ; 29
        WASTE_2                 ; 31
        ____SPKR_DUTY____4      ; 35    Toggle speaker
        STORE_TARGET_3          ; 38
        WASTE_2                 ; 40
        JMP_NEXT_6              ; 46
:
        KBD_LOAD_7              ;    27
        WASTE_4                 ;    31
        ____SPKR_DUTY____4      ;    35 Toggle speaker
        WASTE_8                 ;    43
        jmp     duty_cycle27    ;    46
