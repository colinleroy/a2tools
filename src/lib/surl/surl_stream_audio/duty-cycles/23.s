duty_cycle23:
        DEBUG_JMP   #'N'
        ____SPKR_DUTY____4      ; 4     Toggle speaker

        lda     #INV_SPC        ; 6    Set VU meter
v23a:   sta     txt_level       ; 10

s23:    lda     ser_status      ; 14    Check serial
        and     has_byte        ; 17
        beq     :+              ; 19/20
d23:    ldx     ser_data        ; 23    Load serial

        lda     #SPC            ; 25    Unset VU meter
        WASTE_2                 ; 27

        ____SPKR_DUTY____4      ; 31    Toggle speaker
        STORE_TARGET_3          ; 34
v23b:   sta     txt_level       ; 38
        WASTE_2                 ; 40
        JMP_NEXT_6              ; 46
:
        WASTE_7                 ;    27
        ____SPKR_DUTY____4      ;    31 Toggle speaker
        WASTE_5                 ;    36
        KBD_LOAD_7              ;    43
        jmp     duty_cycle23    ;    46
