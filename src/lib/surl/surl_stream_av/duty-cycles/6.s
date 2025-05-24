duty_cycle6:                    ; end spkr at 14
        ____SPKR_DUTY____4      ; 4
ad6:    ldx     $A8FF           ; 8
        WASTE_2                 ; 10
        ____SPKR_DUTY____4      ; 14
vs6:    lda     $99FF           ; 18
        and     has_byte        ; 21
        beq     no_vid6         ; 23/24
vd6:    ldy     $98FF           ; 27
        WASTE_5                 ; 32
        PREPARE_VIDEO_7         ; 39
        jmp     video_sub       ; 42=>68

no_vid6:
ad6b:   ldx     $A8FF           ; 28
        WASTE_31                ; 59
        JUMP_NEXT_9             ; 68
