duty_cycle0:                    ; Max negative level, 8 cycles
        ____SPKR_DUTY____4      ; 4     Toggle speaker
        ____SPKR_DUTY____4      ; 8     Toggle speaker
        lda     #INV_SPC        ; 10    Set VU meter
v0a:    sta     txt_level       ; 14
        
s0:     lda     ser_status      ; 18    Check serial
        and     #HAS_BYTE       ; 20

        beq     :+              ; 22/23

d0:     ldx     ser_data        ; 26    Load serial

        lda     #SPC            ; 28    Unset VU meter
        WASTE_7                 ; 35

v0b:    sta     txt_level       ; 39
        JMP_NEXT_6              ; 45
:
        WASTE_12                ;    35
        KBD_LOAD_7              ;    42
        jmp     duty_cycle0     ;    45
