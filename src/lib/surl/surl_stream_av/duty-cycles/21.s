duty_cycle21:                    ; end spkr at 29
        ____SPKR_DUTY____4      ; 4
ad21:   ldx     $A8FF           ; 8
vs21:   lda     $99FF           ; 12
        and     #HAS_BYTE       ; 14
        beq     no_vid21        ; 16/17
vd21:   ldy     $98FF           ; 20
        WASTE_5                 ; 25
        ____SPKR_DUTY____4      ; 29
        WASTE_3                 ; 32
        PREPARE_VIDEO_7         ; 39
        jmp     video_sub       ; 42=>68

no_vid21:
ad21b:  ldx     $A8FF           ; 21
        WASTE_4                 ; 25
        ____SPKR_DUTY____4      ; 29
        WASTE_27                ; 56
        JUMP_NEXT_12            ; 68
