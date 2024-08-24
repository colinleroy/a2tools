duty_cycle25:                    ; end spkr at 33
        ____SPKR_DUTY____4      ; 4
ad25:   ldx     $A8FF           ; 8
vs25:   lda     $99FF           ; 12
        and     #HAS_BYTE       ; 14
        beq     no_vid25        ; 16/17
vd25:   ldy     $98FF           ; 20
        WASTE_2                 ; 22
        PREPARE_VIDEO_7         ; 29
        ____SPKR_DUTY____4      ; 33
        WASTE_6                 ; 39
        jmp     video_sub       ; 42=>68

no_vid25:
ad25b:  ldx     $A8FF           ; 21
        WASTE_8                 ; 29
        ____SPKR_DUTY____4      ; 33
        WASTE_23                ; 56
        JUMP_NEXT_12            ; 68
