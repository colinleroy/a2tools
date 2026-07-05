duty_cycle6:
        DEBUG_JMP   #'6'
        ____SPKR_DUTY____4      ; 4     Toggle speaker

        lda     #INV_SPC        ; 6    Set VU meter
v6a:    sta     txt_level       ; 10
        ____SPKR_DUTY____4      ; 14    Toggle speaker
        
s6:     lda     ser_status      ; 18    Check serial
        and     has_byte        ; 21
        beq     :+              ; 23/24

d6:     ldx     ser_data        ; 27    Load serial

        lda     #SPC            ; 29    Unset VU meter
        STORE_TARGET_3          ; 32
        WASTE_4                 ; 36

v6b:    sta     txt_level       ; 40
        JMP_NEXT_6              ; 46
:
        WASTE_12                ;    36
        KBD_LOAD_7              ;    43
        jmp     duty_cycle6     ;    46
