duty_cycle20:
        DEBUG_JMP   #'K'
        ____SPKR_DUTY____4      ; 4     Toggle speaker

s20:    lda     ser_status      ; 8    Check serial
        and     has_byte        ; 11
        bne     d20             ; 13/14 - Inverted for 1 cycle waste

        WASTE_11                ;    24
        ____SPKR_DUTY____4      ;    28 Toggle speaker
        WASTE_7                 ;    35
        KBD_LOAD_7              ;    42
        jmp     duty_cycle20    ;    45

d20:    ldx     ser_data        ; 18    Load serial
        lda     #INV_SPC        ; 20    Set VU meter
v20a:   sta     txt_level       ; 24

        ____SPKR_DUTY____4      ; 28    Toggle speaker
        STORE_TARGET_3          ; 31

        lda     #SPC            ; 33    Unset VU meter
v20b:   sta     txt_level       ; 37
        WASTE_2                 ; 39
        JMP_NEXT_6              ; 45
