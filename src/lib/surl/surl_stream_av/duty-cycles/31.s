duty_cycle31:                    ; end spkr at 39
        ____SPKR_DUTY____4      ; 4
ad31:   ldx     $A8FF           ; 8
vs31:   lda     $99FF           ; 12
        and     has_byte        ; 15
        beq     no_vid31        ; 17/18
vd31:   ldy     $98FF           ; 21
        WASTE_7                 ; 28
        PREPARE_VIDEO_7         ; 35
        ____SPKR_DUTY____4      ; 39
        jmp     video_sub       ; 42=>68

no_vid31:
ad31b:  ldx     $A8FF           ; 22
        WASTE_13                ; 35
        ____SPKR_DUTY____4      ; 39
        WASTE_20                ; 59
        JUMP_NEXT_9             ; 68
