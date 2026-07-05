duty_cycle31:
        DEBUG_JMP   #'V'
        ____SPKR_DUTY____4      ; 4     Toggle speaker

s31:    lda     ser_status      ; 8     Check serial
        and     has_byte        ; 11
        beq     :+              ; 13/14
d31:    ldx     ser_data        ; 17    Load serial

.ifdef STREAMER_65C02
        lda     next,x          ; 21    Update target
        sta     dest31+1        ; 25
        inx                     ; 27
        lda     next,x          ; 31

        sta     dest31+2        ; 35    Finish updating target
.else
        WASTE_11                ; 28
        WASTE_3                 ; 31
        stx dest31+2            ; 35
.endif
        ____SPKR_DUTY____4      ; 39    Toggle speaker
        WASTE_4                 ; 43
dest31: jmp     $0000           ; 46

:
        lda     #INV_SPC        ;    16 Set VU meter
v31a:   sta     txt_level       ;    20
        KBD_LOAD_7              ;    27
        WASTE_6                 ;    33
        lda     #SPC            ;    35
        ____SPKR_DUTY____4      ;    39 Toggle speaker
v31b:   sta     txt_level       ;    43 Unset VU meter
        jmp     duty_cycle31    ;    46
