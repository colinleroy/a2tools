duty_cycle24:                    ; end spkr at 32
        ____SPKR_DUTY____4      ; 4
ad24:   ldx     $A8FF           ; 8
vs24:   lda     $99FF           ; 12
        and     has_byte        ; 15
        beq     no_vid24        ; 17/18
vd24:   ldy     $98FF           ; 21
        PREPARE_VIDEO_7         ; 28
        ____SPKR_DUTY____4      ; 32
        WASTE_7                 ; 39
        jmp     video_sub       ; 42=>68

no_vid24:
ad24b:  ldx     $A8FF           ; 22
        WASTE_6                 ; 28
        ____SPKR_DUTY____4      ; 32
        WASTE_27                ; 59
        JUMP_NEXT_9             ; 68
