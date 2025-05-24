duty_cycle19:
        ____SPKR_DUTY____4      ; 4     Toggle speaker

        lda     #INV_SPC        ; 6    Set VU meter
v19a:   sta     txt_level       ; 10

s19:    lda     ser_status      ; 14    Check serial
        and     has_byte        ; 17
        beq     :+              ; 19/20
d19:    ldx     ser_data        ; 23    Load serial
        ____SPKR_DUTY____4      ; 27    Toggle speaker
        lda     #SPC            ; 29    Unset VU meter
        STORE_TARGET_3          ; 32
        WASTE_3                 ; 35

v19b:   sta     txt_level       ; 39
        JMP_NEXT_6              ; 45
:
        WASTE_3                 ;    23
        ____SPKR_DUTY____4      ;    27 Toggle speaker
        WASTE_8                 ;    35
        KBD_LOAD_7              ;    42
        jmp     duty_cycle19    ;    45
