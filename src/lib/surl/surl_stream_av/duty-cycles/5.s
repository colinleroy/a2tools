duty_cycle5:                    ; end spkr at 13
        ____SPKR_DUTY____4      ; 4
        WASTE_5                 ; 9
        ____SPKR_DUTY____4      ; 13
ad5:    ldx     $A8FF           ; 17
vs5:    lda     $99FF           ; 21
        and     has_byte        ; 24
        beq     no_vid5         ; 26/27
vd5:    ldy     $98FF           ; 30
        WASTE_2                 ; 32
        PREPARE_VIDEO_7         ; 39
        jmp     video_sub       ; 42=>68

no_vid5:
ad5b:   ldx     $A8FF           ; 31
        WASTE_28                ; 59
        STORE_JUMP_TGT_3        ; 62
        JUMP_NEXT_6             ; 68
