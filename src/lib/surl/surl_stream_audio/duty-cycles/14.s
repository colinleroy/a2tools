duty_cycle14:
        DEBUG_JMP   #'E'
        ____SPKR_DUTY____4      ; 4     Toggle speaker

        lda     #INV_SPC        ; 6    Set VU meter
v14a:   sta     txt_level       ; 10

s14:    lda     ser_status      ; 14    Check serial
        and     has_byte        ; 17
        ____SPKR_DUTY____5 14   ; 22    Toggle speaker

        beq     :+              ; 24/25

d14:    ldx     ser_data        ; 28    Load serial

        lda     #SPC            ; 30    Unset VU meter
        STORE_TARGET_3          ; 33
        WASTE_3                 ; 36

v14b:   sta     txt_level       ; 40
        JMP_NEXT_6              ; 46
:
        WASTE_11                ;    36
        KBD_LOAD_7              ;    43
        jmp     duty_cycle14    ;    46
