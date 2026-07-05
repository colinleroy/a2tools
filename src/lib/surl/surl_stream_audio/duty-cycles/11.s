duty_cycle11:
        DEBUG_JMP   #'B'
        ____SPKR_DUTY____4      ; 4     Toggle speaker

        lda     #INV_SPC        ; 6    Set VU meter
v11a:   sta     txt_level       ; 10

s11:    lda     ser_status      ; 14    Check serial
        ____SPKR_DUTY____5 11   ; 19    Toggle speaker
        and     has_byte        ; 22

        beq     :+              ; 24/25

d11:    ldx     ser_data        ; 28    Load serial

        lda     #SPC            ; 30    Unset VU meter
        STORE_TARGET_3          ; 33
        WASTE_3                 ; 36

v11b:   sta     txt_level       ; 40
        JMP_NEXT_6              ; 46
:
        WASTE_11                ;    36
        KBD_LOAD_7              ;    43
        jmp     duty_cycle11    ;    46
