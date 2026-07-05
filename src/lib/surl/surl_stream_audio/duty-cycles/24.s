duty_cycle24:
        DEBUG_JMP   #'O'
        ____SPKR_DUTY____4      ; 4     Toggle speaker

        lda     #INV_SPC        ; 6    Set VU meter
v24a:   sta     txt_level       ; 10

s24:    lda     ser_status      ; 14    Check serial
        and     has_byte        ; 17
        beq     :+              ; 19/20
d24:    ldx     ser_data        ; 23    Load serial

        lda     #SPC            ; 25    Unset VU meter
        STORE_TARGET_3          ; 28

        ____SPKR_DUTY____4      ; 32    Toggle speaker
        WASTE_4                 ; 36
v24b:   sta     txt_level       ; 40
        JMP_NEXT_6              ; 46
:
        WASTE_8                 ;    28
        ____SPKR_DUTY____4      ;    32 Toggle speaker
        WASTE_4                 ;    36
        KBD_LOAD_7              ;    43
        jmp     duty_cycle24    ;    46
