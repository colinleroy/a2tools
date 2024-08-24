duty_cycle22:                    ; end spkr at 30
        ____SPKR_DUTY____4      ; 4
ad22:   ldx     $A8FF           ; 8
vs22:   lda     $99FF           ; 12
        and     #HAS_BYTE       ; 14
        beq     no_vid22        ; 16/17
vd22:   ldy     $98FF           ; 20
        WASTE_6                 ; 26
        ____SPKR_DUTY____4      ; 30
        WASTE_2                 ; 32
        PREPARE_VIDEO_7         ; 39
        jmp     video_sub       ; 42=>68

no_vid22:
ad22b:  ldx     $A8FF           ; 21
        WASTE_5                 ; 26
        ____SPKR_DUTY____4      ; 30
        WASTE_26                ; 56
        JUMP_NEXT_12            ; 68
