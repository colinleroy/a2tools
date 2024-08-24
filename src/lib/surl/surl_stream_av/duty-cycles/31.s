duty_cycle31:                    ; end spkr at 39
        ____SPKR_DUTY____4      ; 4
ad31:   ldx     $A8FF           ; 8
vs31:   lda     $99FF           ; 12
        and     #HAS_BYTE       ; 14
        beq     no_vid31        ; 16/17
vd31:   ldy     $98FF           ; 20
        WASTE_8                 ; 28
        PREPARE_VIDEO_7         ; 35
        ____SPKR_DUTY____4      ; 39
        jmp     video_sub       ; 42=>68

no_vid31:
ad31b:  ldx     $A8FF           ; 21
        WASTE_14                ; 35
        ____SPKR_DUTY____4      ; 39
        WASTE_17                ; 56
        JUMP_NEXT_12            ; 68
