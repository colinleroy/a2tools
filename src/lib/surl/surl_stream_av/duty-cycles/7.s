duty_cycle7:                    ; end spkr at 15
        ____SPKR_DUTY____4      ; 4
ad7:    ldx     $A8FF           ; 8
        WASTE_3                 ; 11
        ____SPKR_DUTY____4      ; 15
vs7:    lda     $99FF           ; 19
        and     has_byte        ; 22
        beq     no_vid7         ; 24/25
vd7:    ldy     $98FF           ; 28
        WASTE_4                 ; 32
        PREPARE_VIDEO_7         ; 39
        jmp     video_sub       ; 42=>68

no_vid7:
ad7b:   ldx     $A8FF           ; 29
        WASTE_30                ; 59
        JUMP_NEXT_9             ; 68
