duty_cycle13:                    ; end spkr at 21
        ____SPKR_DUTY____4      ; 4
ad13:   ldx     $A8FF           ; 8
vs13:   lda     $99FF           ; 12
        and     #HAS_BYTE       ; 14
        WASTE_3                 ; 17
        ____SPKR_DUTY____4      ; 21
        beq     no_vid13        ; 23/24
vd13:   ldy     $98FF           ; 27
        WASTE_5                 ; 32
        PREPARE_VIDEO_7         ; 39
        jmp     video_sub       ; 42=>68

no_vid13:
ad13b:  ldx     $A8FF           ; 28
        WASTE_28                ; 56
        JUMP_NEXT_12            ; 68
