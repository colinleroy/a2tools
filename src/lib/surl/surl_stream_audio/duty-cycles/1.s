duty_cycle1:
        ____SPKR_DUTY____4      ; 4     Toggle speaker
        ____SPKR_DUTY____5 1    ; 9    Toggle speaker
        lda     #INV_SPC        ; 11    Set VU meter
v1a:    sta     txt_level       ; 15
        
s1:     lda     ser_status      ; 19    Check serial
        and     #HAS_BYTE       ; 21

        beq     :+              ; 23/24

d1:     ldx     ser_data        ; 27    Load serial

        lda     #SPC            ; 29    Unset VU meter
        WASTE_6                 ; 35

v1b:    sta     txt_level       ; 39
        JMP_NEXT_6              ; 45
:
        WASTE_11                ;    35
        KBD_LOAD_7              ;    42
        jmp     duty_cycle1     ;    45
