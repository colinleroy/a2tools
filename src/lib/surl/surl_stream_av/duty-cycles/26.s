duty_cycle26:                    ; end spkr at 34
        ____SPKR_DUTY____4      ; 4
ad26:   ldx     $A8FF           ; 8
vs26:   lda     $99FF           ; 12
        and     #HAS_BYTE       ; 14
        beq     no_vid26        ; 16/17
vd26:   ldy     $98FF           ; 20
        WASTE_3                 ; 23
        PREPARE_VIDEO_7         ; 30
        ____SPKR_DUTY____4      ; 34
        WASTE_5                 ; 39
        jmp     video_sub       ; 42=>68

no_vid26:
ad26b:  ldx     $A8FF           ; 21
        WASTE_9                 ; 30
        ____SPKR_DUTY____4      ; 34
        WASTE_25                ; 59
        JUMP_NEXT_9             ; 68
