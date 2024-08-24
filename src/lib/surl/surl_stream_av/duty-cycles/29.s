duty_cycle29:                    ; end spkr at 37
        ____SPKR_DUTY____4      ; 4
ad29:   ldx     $A8FF           ; 8
vs29:   lda     $99FF           ; 12
        and     #HAS_BYTE       ; 14
        beq     no_vid29        ; 16/17
vd29:   ldy     $98FF           ; 20
        WASTE_6                 ; 26
        PREPARE_VIDEO_7         ; 33
        ____SPKR_DUTY____4      ; 37
        WASTE_2                 ; 39
        jmp     video_sub       ; 42=>68

no_vid29:
ad29b:  ldx     $A8FF           ; 21
        WASTE_12                ; 33
        ____SPKR_DUTY____4      ; 37
        WASTE_19                ; 56
        JUMP_NEXT_12            ; 68
