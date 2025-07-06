duty_cycle2:                    ; end spkr at 10
        ____SPKR_DUTY____4      ; 4
        WASTE_2                 ; 6
        ____SPKR_DUTY____4      ; 10
ad2:    ldx     $A8FF           ; 14
vs2:    lda     $99FF           ; 18
        and     has_byte        ; 21
        beq     no_vid2         ; 23/24
vd2:    ldy     $98FF           ; 27
        WASTE_5                 ; 32
        PREPARE_VIDEO_7         ; 39
        jmp     video_sub       ; 42=>68

no_vid2:
ad2b:   ldx     $A8FF           ; 28
        WASTE_31                ; 59
        STORE_JUMP_TGT_3        ; 62
        JUMP_NEXT_6             ; 68
