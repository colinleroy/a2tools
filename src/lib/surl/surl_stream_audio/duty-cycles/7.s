duty_cycle7:
        ____SPKR_DUTY____4      ; 4     Toggle speaker

        lda     #INV_SPC        ; 6    Set VU meter
v7a:    sta     txt_level       ; 10
        ____SPKR_DUTY____5 7    ; 15    Toggle speaker
        
s7:     lda     ser_status      ; 19    Check serial
        and     #HAS_BYTE       ; 21

        beq     :+              ; 23/24

d7:     ldx     ser_data        ; 27    Load serial

        lda     #SPC            ; 29    Unset VU meter
        WASTE_6                 ; 35

v7b:    sta     txt_level       ; 39
        JMP_NEXT_6              ; 45
:
        WASTE_11                ;    35
        KBD_LOAD_7              ;    42
        jmp     duty_cycle7     ;    45
