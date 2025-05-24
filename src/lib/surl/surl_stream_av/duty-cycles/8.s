duty_cycle8:                    ; end spkr at 16
        ____SPKR_DUTY____4      ; 4
ad8:    ldx     $A8FF           ; 8
vs8:    lda     $99FF           ; 12
        ____SPKR_DUTY____4      ; 16
        and     has_byte        ; 19
        beq     no_vid8         ; 21/22
vd8:    ldy     $98FF           ; 25
        WASTE_7                 ; 32
        PREPARE_VIDEO_7         ; 39
        jmp     video_sub       ; 42=>68

no_vid8:
ad8b:   ldx     $A8FF           ; 26
        WASTE_33                ; 59
        JUMP_NEXT_9             ; 68
