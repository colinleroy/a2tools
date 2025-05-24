duty_cycle10:
        ____SPKR_DUTY____4      ; 4     Toggle speaker

        lda     #INV_SPC        ; 6    Set VU meter
v10a:   sta     txt_level       ; 10

s10:    lda     ser_status      ; 14    Check serial
        ____SPKR_DUTY____4      ; 18    Toggle speaker
        and     has_byte        ; 21

        beq     :+              ; 23/24

d10:    ldx     ser_data        ; 27    Load serial

        lda     #SPC            ; 29    Unset VU meter
        STORE_TARGET_3          ; 32
        WASTE_3                 ; 35

v10b:   sta     txt_level       ; 39
        JMP_NEXT_6              ; 45
:
        WASTE_11                ;    35
        KBD_LOAD_7              ;    42
        jmp     duty_cycle10    ;    45
