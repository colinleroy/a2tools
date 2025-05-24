duty_cycle21:                    ; end spkr at 29
        ____SPKR_DUTY____4      ; 4
ad21:   ldx     $A8FF           ; 8
vs21:   lda     $99FF           ; 12
        and     has_byte        ; 15
        beq     no_vid21        ; 17/18
vd21:   ldy     $98FF           ; 21
        WASTE_4                 ; 25
        ____SPKR_DUTY____4      ; 29
        WASTE_3                 ; 32
        PREPARE_VIDEO_7         ; 39
        jmp     video_sub       ; 42=>68

no_vid21:
ad21b:  ldx     $A8FF           ; 22
        WASTE_3                 ; 25
        ____SPKR_DUTY____4      ; 29
        WASTE_30                ; 59
        JUMP_NEXT_9             ; 68
