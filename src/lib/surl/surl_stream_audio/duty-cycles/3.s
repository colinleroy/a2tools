duty_cycle3:
        ____SPKR_DUTY____4      ; 4     Toggle speaker

        lda     #INV_SPC        ; 6    Set VU meter
        ____SPKR_DUTY____5 3    ; 11    Toggle speaker
v3a:    sta     txt_level       ; 15
        
s3:     lda     ser_status      ; 19    Check serial
        and     #HAS_BYTE       ; 21

        beq     :+              ; 23/24

d3:     ldx     ser_data        ; 27    Load serial

        lda     #SPC            ; 29    Unset VU meter
        WASTE_6                 ; 35

v3b:    sta     txt_level       ; 39
        JMP_NEXT_6              ; 45
:
        WASTE_11                ;    35
        KBD_LOAD_7              ;    42
        jmp     duty_cycle3     ;    45
