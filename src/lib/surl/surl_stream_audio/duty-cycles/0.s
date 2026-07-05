duty_cycle0:                    ; Max negative level, 8 cycles
        DEBUG_JMP   #'0'
        ____SPKR_DUTY____4      ; 4     Toggle speaker
        ____SPKR_DUTY____4      ; 8     Toggle speaker
        lda     #INV_SPC        ; 10    Set VU meter
v0a:    sta     txt_level       ; 14
        
s0:     lda     ser_status      ; 18    Check serial
        and     has_byte        ; 21

        beq     :+              ; 23/24

d0:     ldx     ser_data        ; 27    Load serial

        lda     #SPC            ; 29    Unset VU meter
        STORE_TARGET_3          ; 32
        WASTE_4                 ; 36

v0b:    sta     txt_level       ; 40
        JMP_NEXT_6              ; 46
:
        WASTE_12                ;    36
        KBD_LOAD_7              ;    43
        jmp     duty_cycle0     ;    46
