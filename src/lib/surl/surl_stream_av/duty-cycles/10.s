duty_cycle10:                    ; end spkr at 18
        ____SPKR_DUTY____4      ; 4
ad10:   ldx     $A8FF           ; 8
vs10:   lda     $99FF           ; 12
        WASTE_2                 ; 14
        ____SPKR_DUTY____4      ; 18
        and     #HAS_BYTE       ; 20
        beq     no_vid10        ; 22/23
vd10:   ldy     $98FF           ; 26
        WASTE_6                 ; 32
        PREPARE_VIDEO_7         ; 39
        jmp     video_sub       ; 42=>68

no_vid10:
ad10b:  ldx     $A8FF           ; 27
        WASTE_32                ; 59
        JUMP_NEXT_9             ; 68
