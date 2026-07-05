duty_cycle7:
        DEBUG_JMP   #'7'
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
        WASTE_3                 ; 36

v7b:    sta     txt_level       ; 40
        JMP_NEXT_6              ; 46
:
        WASTE_11                ;    36
        KBD_LOAD_7              ;    43
        jmp     duty_cycle7     ;    46
