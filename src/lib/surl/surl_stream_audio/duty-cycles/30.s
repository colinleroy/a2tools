duty_cycle30:
        ____SPKR_DUTY____4      ; 4     Toggle speaker

s30:    lda     ser_status      ; 8     Check serial
        and     #HAS_BYTE       ; 10
        beq     :+              ; 12/13
d30:    ldx     ser_data        ; 16    Load serial

.ifdef __APPLE2ENH__
        lda     next,x          ; 20    Update target
        sta     dest30+1        ; 24
        inx                     ; 26
        lda     next,x          ; 30

        WASTE_4                 ; 34
        ____SPKR_DUTY____4      ; 38    Toggle speaker
        sta     dest30+2        ; 42    Finish updating target
.else
        WASTE_12                ; 28
        WASTE_6                 ; 34
        ____SPKR_DUTY____4      ; 38    Toggle speaker
        stx dest30+2            ; 42
.endif
dest30: jmp     $0000           ; 45

:
        lda     #INV_SPC        ;    15 Set VU meter
v30a:   sta     txt_level       ;    19
        KBD_LOAD_7              ;    26
        WASTE_6                 ;    32
        lda     #SPC            ;    34
        ____SPKR_DUTY____4      ;    38 Toggle speaker
v30b:   sta     txt_level       ;    42 Unset VU meter
        jmp     duty_cycle30    ;    45
