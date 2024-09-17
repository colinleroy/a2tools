duty_cycle2:                    ; end spkr at 10
        ____SPKR_DUTY____4      ; 4
        WASTE_2                 ; 6
        ____SPKR_DUTY____4      ; 10
ad2:    ldx     $A8FF           ; 14
vs2:    lda     $99FF           ; 18
        and     #HAS_BYTE       ; 20
        beq     no_vid2         ; 22/23
vd2:    ldy     $98FF           ; 26
        WASTE_6                 ; 32
        PREPARE_VIDEO_7         ; 39
        jmp     video_sub       ; 42=>68

no_vid2:
ad2b:   ldx     $A8FF           ; 27
        WASTE_32                ; 59
        JUMP_NEXT_9             ; 68
