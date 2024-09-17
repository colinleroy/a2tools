duty_cycle8:                    ; end spkr at 16
        ____SPKR_DUTY____4      ; 4
ad8:    ldx     $A8FF           ; 8
vs8:    lda     $99FF           ; 12
        ____SPKR_DUTY____4      ; 16
        and     #HAS_BYTE       ; 18
        beq     no_vid8         ; 20/21
vd8:    ldy     $98FF           ; 24
        WASTE_8                 ; 32
        PREPARE_VIDEO_7         ; 39
        jmp     video_sub       ; 42=>68

no_vid8:
ad8b:   ldx     $A8FF           ; 25
        WASTE_34                ; 59
        JUMP_NEXT_9             ; 68
