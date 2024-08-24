duty_cycle0:                    ; end spkr at 8
        ____SPKR_DUTY____4      ; 4      toggle speaker on
        ____SPKR_DUTY____4      ; 8      toggle speaker off
vs0:    lda     $99FF           ; 12     load video status register
ad0:    ldx     $A8FF           ; 16     load audio data register
        and     #HAS_BYTE       ; 18     check if video has byte
        beq     no_vid0         ; 20/21  branch accordingly
vd0:    ldy     $98FF           ; 24     load video data
        WASTE_8                 ; 32     waste extra cycles
        PREPARE_VIDEO_7         ; 39
        jmp     video_sub       ; 42=>68

no_vid0:                        ;        we had no video byte
ad0b:   ldx     $A8FF           ; 25     load audio data register again
        WASTE_31                ; 56
        JUMP_NEXT_12            ; 68
