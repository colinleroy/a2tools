duty_cycle5:                    ; end spkr at 13
        ____SPKR_DUTY____4      ; 4
        WASTE_5                 ; 9
        ____SPKR_DUTY____4      ; 13
ad5:    ldx     $A8FF           ; 17
vs5:    lda     $99FF           ; 21
        and     #HAS_BYTE       ; 23
        beq     no_vid5         ; 25/26
vd5:    ldy     $98FF           ; 29
        WASTE_3                 ; 32
        PREPARE_VIDEO_7         ; 39
        jmp     video_sub       ; 42=>68

no_vid5:
ad5b:   ldx     $A8FF           ; 30
        WASTE_26                ; 56
        JUMP_NEXT_12            ; 68
