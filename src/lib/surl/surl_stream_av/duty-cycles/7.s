duty_cycle7:                    ; end spkr at 15
        ____SPKR_DUTY____4      ; 4
ad7:    ldx     $A8FF           ; 8
        WASTE_3                 ; 11
        ____SPKR_DUTY____4      ; 15
vs7:    lda     $99FF           ; 19
        and     #HAS_BYTE       ; 21
        beq     no_vid7         ; 23/24
vd7:    ldy     $98FF           ; 27
        WASTE_5                 ; 32
        PREPARE_VIDEO_7         ; 39
        jmp     video_sub       ; 42=>68

no_vid7:
ad7b:   ldx     $A8FF           ; 28
        WASTE_28                ; 56
        JUMP_NEXT_12            ; 68
