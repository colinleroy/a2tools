duty_cycle1:
        DEBUG_JMP   #'1'
        ____SPKR_DUTY____4      ; 4     Toggle speaker
        ____SPKR_DUTY____5 1    ; 9    Toggle speaker
        lda     #INV_SPC        ; 11    Set VU meter
v1a:    sta     txt_level       ; 15
        
s1:     lda     ser_status      ; 19    Check serial
        and     has_byte        ; 22

        beq     :+              ; 24/25

d1:     ldx     ser_data        ; 28    Load serial

        lda     #SPC            ; 30    Unset VU meter
        STORE_TARGET_3          ; 33
        WASTE_3                 ; 36

v1b:    sta     txt_level       ; 40
        JMP_NEXT_6              ; 46
:
        WASTE_11                ;    36
        KBD_LOAD_7              ;    43
        jmp     duty_cycle1     ;    46
