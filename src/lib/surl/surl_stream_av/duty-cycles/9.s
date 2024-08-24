duty_cycle9:                    ; end spkr at 17
        ____SPKR_DUTY____4      ; 4
ad9:    ldx     $A8FF           ; 8
        WASTE_5                 ; 13
        ____SPKR_DUTY____4      ; 17
vs9:    lda     $99FF           ; 21
        and     #HAS_BYTE       ; 23
        beq     no_vid9         ; 25/26
vd9:    ldy     $98FF           ; 29
        WASTE_3                 ; 32
        PREPARE_VIDEO_7         ; 39
        jmp     video_sub       ; 42=>68

no_vid9:
ad9b:   ldx     $A8FF           ; 30
        WASTE_26                ; 56
        JUMP_NEXT_12            ; 68
