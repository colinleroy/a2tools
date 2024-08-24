duty_cycle27:                    ; end spkr at 35
        ____SPKR_DUTY____4      ; 4
ad27:   ldx     $A8FF           ; 8
vs27:   lda     $99FF           ; 12
        and     #HAS_BYTE       ; 14
        beq     no_vid27        ; 16/17
vd27:   ldy     $98FF           ; 20
        WASTE_4                 ; 24
        PREPARE_VIDEO_7         ; 31
        ____SPKR_DUTY____4      ; 35
        WASTE_4                 ; 39
        jmp     video_sub       ; 42=>68

no_vid27:
ad27b:  ldx     $A8FF           ; 21
        WASTE_10                 ; 31
        ____SPKR_DUTY____4      ; 35
        WASTE_21                ; 56
        JUMP_NEXT_12            ; 68
