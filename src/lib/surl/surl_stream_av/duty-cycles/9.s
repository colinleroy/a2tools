duty_cycle9:                    ; end spkr at 17
        ____SPKR_DUTY____4      ; 4
ad9:    ldx     $A8FF           ; 8
        WASTE_5                 ; 13
        ____SPKR_DUTY____4      ; 17
vs9:    lda     $99FF           ; 21
        and     has_byte        ; 24
        beq     no_vid9         ; 26/27
vd9:    ldy     $98FF           ; 30
        WASTE_2                 ; 32
        PREPARE_VIDEO_7         ; 39
        jmp     video_sub       ; 42=>68

no_vid9:
ad9b:   ldx     $A8FF           ; 31
        WASTE_28                ; 59
        JUMP_NEXT_9             ; 68
