duty_cycle6:                    ; end spkr at 14
        ____SPKR_DUTY____4      ; 4
ad6:    ldx     $A8FF           ; 8
        WASTE_2                 ; 10
        ____SPKR_DUTY____4      ; 14
vs6:    lda     $99FF           ; 18
        and     #HAS_BYTE       ; 20
        beq     no_vid6         ; 22/23
vd6:    ldy     $98FF           ; 26
        WASTE_6                 ; 32
        PREPARE_VIDEO_7         ; 39
        jmp     video_sub       ; 42=>68

no_vid6:
ad6b:   ldx     $A8FF           ; 27
        WASTE_32                ; 59
        JUMP_NEXT_9             ; 68
