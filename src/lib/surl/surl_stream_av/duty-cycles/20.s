duty_cycle20:                    ; end spkr at 28
        ____SPKR_DUTY____4      ; 4
ad20:   ldx     $A8FF           ; 8
vs20:   lda     $99FF           ; 12
        and     has_byte        ; 15
        beq     no_vid20        ; 17/18
vd20:   ldy     $98FF           ; 21
        WASTE_3                 ; 24
        ____SPKR_DUTY____4      ; 28
        WASTE_4                 ; 32
        PREPARE_VIDEO_7         ; 39
        jmp     video_sub       ; 42=>68

no_vid20:
ad20b:  ldx     $A8FF           ; 22
        WASTE_2                 ; 24
        ____SPKR_DUTY____4      ; 28
        WASTE_31                ; 59
        JUMP_NEXT_9             ; 68
