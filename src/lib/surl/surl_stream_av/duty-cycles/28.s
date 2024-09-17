duty_cycle28:                    ; end spkr at 36
        ____SPKR_DUTY____4      ; 4
ad28:   ldx     $A8FF           ; 8
vs28:   lda     $99FF           ; 12
        and     #HAS_BYTE       ; 14
        beq     no_vid28        ; 16/17
vd28:   ldy     $98FF           ; 20
        WASTE_5                 ; 25
        PREPARE_VIDEO_7         ; 32
        ____SPKR_DUTY____4      ; 36
        WASTE_3                 ; 39
        jmp     video_sub       ; 42=>68

no_vid28:
ad28b:  ldx     $A8FF           ; 21
        WASTE_11                ; 32
        ____SPKR_DUTY____4      ; 36
        WASTE_23                ; 59
        JUMP_NEXT_9             ; 68
