duty_cycle7:
        ____SPKR_DUTY____4      ; 4     Toggle speaker

        lda     #INV_SPC        ; 6    Set VU meter
v7a:    sta     txt_level       ; 10
        ____SPKR_DUTY____5 7    ; 15    Toggle speaker
        
s7:     lda     ser_status      ; 19    Check serial
        and     has_byte        ; 22

        beq     :+              ; 24/25

d7:     ldx     ser_data        ; 28    Load serial

        lda     #SPC            ; 30    Unset VU meter
        STORE_TARGET_3          ; 33
        WASTE_2                 ; 35

v7b:    sta     txt_level       ; 39
        JMP_NEXT_6              ; 45
:
        WASTE_10                ;    35
        KBD_LOAD_7              ;    42
        jmp     duty_cycle7     ;    45
