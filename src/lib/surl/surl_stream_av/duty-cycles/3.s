duty_cycle3:                    ; end spkr at 11
        ____SPKR_DUTY____4      ; 4
        WASTE_3                 ; 7
        ____SPKR_DUTY____4      ; 11
ad3:    ldx     $A8FF           ; 15
vs3:    lda     $99FF           ; 19
        and     has_byte        ; 22
        beq     no_vid3         ; 24/25
vd3:    ldy     $98FF           ; 28
        WASTE_4                 ; 32
        PREPARE_VIDEO_7         ; 39
        jmp     video_sub       ; 42=>68

no_vid3:
ad3b:   ldx     $A8FF           ; 29
        WASTE_30                ; 59
        JUMP_NEXT_9             ; 68
