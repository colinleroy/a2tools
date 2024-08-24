duty_cycle19:                    ; end spkr at 27
        ____SPKR_DUTY____4      ; 4
ad19:   ldx     $A8FF           ; 8
vs19:   lda     $99FF           ; 12
        and     #HAS_BYTE       ; 14
        beq     no_vid19        ; 16/17
vd19:   ldy     $98FF           ; 20
        WASTE_3                 ; 23
        ____SPKR_DUTY____4      ; 27
        WASTE_5                 ; 32
        PREPARE_VIDEO_7         ; 39
        jmp     video_sub       ; 42=>68

no_vid19:
ad19b:  ldx     $A8FF           ; 21
        WASTE_2                 ; 23
        ____SPKR_DUTY____4      ; 27
        WASTE_29                ; 56
        JUMP_NEXT_12            ; 68
