duty_cycle4:                    ; end spkr at 12
        ____SPKR_DUTY____4      ; 4
ad4:    ldx     $A8FF           ; 8
        ____SPKR_DUTY____4      ; 12
vs4:    lda     $99FF           ; 16
        and     #HAS_BYTE       ; 18
        beq     no_vid4         ; 20/21
vd4:    ldy     $98FF           ; 24
        WASTE_8                 ; 32
        PREPARE_VIDEO_7         ; 39
        jmp     video_sub       ; 42=>68

no_vid4:
ad4b:   ldx     $A8FF           ; 25
        WASTE_31                ; 56
        JUMP_NEXT_12            ; 68
