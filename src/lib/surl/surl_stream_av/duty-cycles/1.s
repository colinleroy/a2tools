duty_cycle1:                    ; end spkr at 9
        ____SPKR_DUTY____4      ; 4
        ____SPKR_DUTY____5 1    ; 9
ad1:    ldx     $A8FF           ; 13
vs1:    lda     $99FF           ; 17
        and     #HAS_BYTE       ; 19
        beq     no_vid1         ; 21/22
vd1:    ldy     $98FF           ; 25
        WASTE_7                 ; 32
        PREPARE_VIDEO_7         ; 39
        jmp     video_sub       ; 42=>68

no_vid1:
ad1b:   ldx     $A8FF           ; 26
        WASTE_33                ; 59
        JUMP_NEXT_9             ; 68
