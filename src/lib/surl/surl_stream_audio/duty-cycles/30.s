duty_cycle30:
        ____SPKR_DUTY____4      ; 4     Toggle speaker

s30:    lda     ser_status      ; 8     Check serial
        and     has_byte        ; 11
        beq     :+              ; 13/14
d30:    ldx     ser_data        ; 17    Load serial

.ifdef __APPLE2ENH__
        lda     next,x          ; 21    Update target
        sta     dest30+1        ; 25
        inx                     ; 27
        lda     next,x          ; 31

        WASTE_3                 ; 34
        ____SPKR_DUTY____4      ; 38    Toggle speaker
        sta     dest30+2        ; 42    Finish updating target
.else
        WASTE_11                ; 28
        WASTE_6                 ; 34
        ____SPKR_DUTY____4      ; 38    Toggle speaker
        stx dest30+2            ; 42
.endif
dest30: jmp     $0000           ; 45

:
        lda     #INV_SPC        ;    16 Set VU meter
v30a:   sta     txt_level       ;    20
        KBD_LOAD_7              ;    27
        WASTE_5                 ;    32
        lda     #SPC            ;    34
        ____SPKR_DUTY____4      ;    38 Toggle speaker
v30b:   sta     txt_level       ;    42 Unset VU meter
        jmp     duty_cycle30    ;    45
