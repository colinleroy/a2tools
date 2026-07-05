duty_cycle13:
        DEBUG_JMP   #'D'
        ____SPKR_DUTY____4      ; 4     Toggle speaker

        lda     #INV_SPC        ; 6    Set VU meter
v13a:   sta     txt_level       ; 10

s13:    lda     ser_status      ; 14    Check serial
        and     has_byte        ; 17
        ____SPKR_DUTY____4      ; 21    Toggle speaker

        beq     :+              ; 23/24

d13:    ldx     ser_data        ; 27    Load serial

        lda     #SPC            ; 29    Unset VU meter
        STORE_TARGET_3          ; 32
        WASTE_4                 ; 36

v13b:   sta     txt_level       ; 40
        JMP_NEXT_6              ; 46
:
        WASTE_12                ;    36
        KBD_LOAD_7              ;    43
        jmp     duty_cycle13    ;    46
