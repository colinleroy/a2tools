duty_cycle4:                    ; end spkr at 12
        ____SPKR_DUTY____4      ; 4
ad4:    ldx     $A8FF           ; 8
        ____SPKR_DUTY____4      ; 12
vs4:    lda     $99FF           ; 16
        and     has_byte        ; 19
        beq     no_vid4         ; 21/22
vd4:    ldy     $98FF           ; 25
        WASTE_7                 ; 32
        PREPARE_VIDEO_7         ; 39
        jmp     video_sub       ; 42=>68

no_vid4:
ad4b:   ldx     $A8FF           ; 26
        WASTE_33                ; 59
        JUMP_NEXT_9             ; 68
