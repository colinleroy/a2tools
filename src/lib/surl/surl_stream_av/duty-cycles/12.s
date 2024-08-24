duty_cycle12:                    ; end spkr at 20
        ____SPKR_DUTY____4      ; 4
ad12:   ldx     $A8FF           ; 8
vs12:   lda     $99FF           ; 12
        and     #HAS_BYTE       ; 14
        WASTE_2                 ; 16
        ____SPKR_DUTY____4      ; 20
        beq     no_vid12        ; 22/23
vd12:   ldy     $98FF           ; 26
        WASTE_6                 ; 32
        PREPARE_VIDEO_7         ; 39
        jmp     video_sub       ; 42=>68

no_vid12:
ad12b:  ldx     $A8FF           ; 27
        WASTE_29                ; 56
        JUMP_NEXT_12            ; 68
