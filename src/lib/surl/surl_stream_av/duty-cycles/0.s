duty_cycle0:                    ; end spkr at 8
        ____SPKR_DUTY____4      ; 4      toggle speaker on
        ____SPKR_DUTY____4      ; 8      toggle speaker off
vs0:    lda     $99FF           ; 12     load video status register
ad0:    ldx     $A8FF           ; 16     load audio data register
        and     has_byte        ; 19     check if video has byte
        beq     no_vid0         ; 21/22  branch accordingly
vd0:    ldy     $98FF           ; 25     load video data
        WASTE_7                 ; 32     waste extra cycles
        PREPARE_VIDEO_7         ; 39
        jmp     video_sub       ; 42=>68

no_vid0:                        ;        we had no video byte
ad0b:   ldx     $A8FF           ; 26     load audio data register again
        WASTE_33                ; 59
        JUMP_NEXT_9             ; 68
