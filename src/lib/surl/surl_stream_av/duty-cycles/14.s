duty_cycle14:                    ; end spkr at 22
        ____SPKR_DUTY____4      ; 4
ad14:   ldx     $A8FF           ; 8
vs14:   lda     $99FF           ; 12
        and     #HAS_BYTE       ; 14
        WASTE_4                 ; 18
        ____SPKR_DUTY____4      ; 22
        beq     no_vid14        ; 24/25
vd14:   ldy     $98FF           ; 28
        WASTE_4                 ; 32
        PREPARE_VIDEO_7         ; 39
        jmp     video_sub       ; 42=>68

no_vid14:
ad14b:  ldx     $A8FF           ; 29
        WASTE_30                ; 59
        JUMP_NEXT_9             ; 68
