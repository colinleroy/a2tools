duty_cycle20:                    ; end spkr at 28
        ____SPKR_DUTY____4      ; 4
ad20:   ldx     $A8FF           ; 8
vs20:   lda     $99FF           ; 12
        and     #HAS_BYTE       ; 14
        beq     no_vid20        ; 16/17
vd20:   ldy     $98FF           ; 20
        WASTE_4                 ; 24
        ____SPKR_DUTY____4      ; 28
        WASTE_4                 ; 32
        PREPARE_VIDEO_7         ; 39
        jmp     video_sub       ; 42=>68

no_vid20:
ad20b:  ldx     $A8FF           ; 21
        WASTE_3                 ; 24
        ____SPKR_DUTY____4      ; 28
        WASTE_28                ; 56
        JUMP_NEXT_12            ; 68
