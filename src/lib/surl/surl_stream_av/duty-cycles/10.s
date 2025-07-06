duty_cycle10:                    ; end spkr at 18
        ____SPKR_DUTY____4      ; 4
ad10:   ldx     $A8FF           ; 8
vs10:   lda     $99FF           ; 12
        WASTE_2                 ; 14
        ____SPKR_DUTY____4      ; 18
        and     has_byte        ; 21
        beq     no_vid10        ; 23/24
vd10:   ldy     $98FF           ; 27
        WASTE_5                 ; 32
        PREPARE_VIDEO_7         ; 39
        jmp     video_sub       ; 42=>68

no_vid10:
ad10b:  ldx     $A8FF           ; 28
        WASTE_31                ; 59
        STORE_JUMP_TGT_3        ; 62
        JUMP_NEXT_6             ; 68
