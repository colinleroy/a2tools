duty_cycle3:
        ____SPKR_DUTY____4      ; 4     Toggle speaker

        lda     #INV_SPC        ; 6    Set VU meter
        ____SPKR_DUTY____5 3    ; 11    Toggle speaker
v3a:    sta     txt_level       ; 15
        
s3:     lda     ser_status      ; 19    Check serial
        and     has_byte        ; 22

        beq     :+              ; 24/25

d3:     ldx     ser_data        ; 28    Load serial

        lda     #SPC            ; 30    Unset VU meter
        STORE_TARGET_3          ; 33
        WASTE_2                 ; 35

v3b:    sta     txt_level       ; 39
        JMP_NEXT_6              ; 45
:
        WASTE_10                ;    35
        KBD_LOAD_7              ;    42
        jmp     duty_cycle3     ;    45
