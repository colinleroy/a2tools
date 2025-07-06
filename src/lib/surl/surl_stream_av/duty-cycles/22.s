duty_cycle22:                    ; end spkr at 30
        ____SPKR_DUTY____4      ; 4
ad22:   ldx     $A8FF           ; 8
vs22:   lda     $99FF           ; 12
        and     has_byte        ; 15
        beq     no_vid22        ; 17/18
vd22:   ldy     $98FF           ; 21
        WASTE_5                 ; 26
        ____SPKR_DUTY____4      ; 30
        WASTE_2                 ; 32
        PREPARE_VIDEO_7         ; 39
        jmp     video_sub       ; 42=>68

no_vid22:
ad22b:  ldx     $A8FF           ; 22
        WASTE_4                 ; 26
        ____SPKR_DUTY____4      ; 30
        WASTE_29                ; 59
        STORE_JUMP_TGT_3        ; 62
        JUMP_NEXT_6             ; 68
