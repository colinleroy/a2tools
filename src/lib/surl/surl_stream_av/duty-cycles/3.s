duty_cycle3:                    ; end spkr at 11
        ____SPKR_DUTY____4      ; 4
        WASTE_3                 ; 7
        ____SPKR_DUTY____4      ; 11
ad3:    ldx     $A8FF           ; 15
vs3:    lda     $99FF           ; 19
        and     #HAS_BYTE       ; 21
        beq     no_vid3         ; 23/24
vd3:    ldy     $98FF           ; 27
        WASTE_5                 ; 32
        PREPARE_VIDEO_7         ; 39
        jmp     video_sub       ; 42=>68

no_vid3:
ad3b:   ldx     $A8FF           ; 28
        WASTE_28                ; 56
        JUMP_NEXT_12            ; 68
