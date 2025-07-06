duty_cycle29:                    ; end spkr at 37
        ____SPKR_DUTY____4      ; 4
ad29:   ldx     $A8FF           ; 8
vs29:   lda     $99FF           ; 12
        and     has_byte        ; 15
        beq     no_vid29        ; 17/18
vd29:   ldy     $98FF           ; 21
        WASTE_5                 ; 26
        PREPARE_VIDEO_7         ; 33
        ____SPKR_DUTY____4      ; 37
        WASTE_2                 ; 39
        jmp     video_sub       ; 42=>68

no_vid29:
ad29b:  ldx     $A8FF           ; 22
        WASTE_11                ; 33
        ____SPKR_DUTY____4      ; 37
        WASTE_22                ; 59
        STORE_JUMP_TGT_3        ; 62
        JUMP_NEXT_6             ; 68
