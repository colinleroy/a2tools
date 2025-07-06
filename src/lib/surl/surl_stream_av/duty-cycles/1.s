duty_cycle1:                    ; end spkr at 9
        ____SPKR_DUTY____4      ; 4
        ____SPKR_DUTY____5 1    ; 9
ad1:    ldx     $A8FF           ; 13
vs1:    lda     $99FF           ; 17
        and     has_byte        ; 20
        beq     no_vid1         ; 22/23
vd1:    ldy     $98FF           ; 26
        WASTE_6                 ; 32
        PREPARE_VIDEO_7         ; 39
        jmp     video_sub       ; 42=>68

no_vid1:
ad1b:   ldx     $A8FF           ; 27
        WASTE_32                ; 59
        STORE_JUMP_TGT_3        ; 62
        JUMP_NEXT_6             ; 68
