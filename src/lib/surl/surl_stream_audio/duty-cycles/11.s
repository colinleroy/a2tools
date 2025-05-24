duty_cycle11:
        ____SPKR_DUTY____4      ; 4     Toggle speaker

        lda     #INV_SPC        ; 6    Set VU meter
v11a:   sta     txt_level       ; 10

s11:    lda     ser_status      ; 14    Check serial
        ____SPKR_DUTY____5 11   ; 19    Toggle speaker
        and     has_byte        ; 22

        beq     :+              ; 24/25

d11:    ldx     ser_data        ; 28    Load serial

        lda     #SPC            ; 30    Unset VU meter
        STORE_TARGET_3          ; 33
        WASTE_2                 ; 35

v11b:   sta     txt_level       ; 39
        JMP_NEXT_6              ; 45
:
        WASTE_10                ;    35
        KBD_LOAD_7              ;    42
        jmp     duty_cycle11    ;    45
