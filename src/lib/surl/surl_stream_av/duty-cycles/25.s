duty_cycle25:                    ; end spkr at 33
        ____SPKR_DUTY____4      ; 4
ad25:   ldx     $A8FF           ; 8
vs25:   lda     $99FF           ; 12
        and     has_byte        ; 15
        beq     no_vid25        ; 17/18
vd25:   ldy     $98FF           ; 21
        PREPARE_VIDEO_E4        ; 25
        WASTE_4                 ; 29
        ____SPKR_DUTY____4      ; 33
        PREPARE_VIDEO_S3        ; 36
        WASTE_3                 ; 39
        jmp     video_sub       ; 42=>68

no_vid25:
ad25b:  ldx     $A8FF           ; 22
        WASTE_7                 ; 29
        ____SPKR_DUTY____4      ; 33
        WASTE_26                ; 59
        JUMP_NEXT_9             ; 68
