duty_cycle19:                    ; end spkr at 27
        ____SPKR_DUTY____4      ; 4
ad19:   ldx     $A8FF           ; 8
vs19:   lda     $99FF           ; 12
        and     has_byte        ; 15
        beq     no_vid19        ; 17/18
vd19:   ldy     $98FF           ; 21
        WASTE_2                 ; 23
        ____SPKR_DUTY____4      ; 27
        WASTE_5                 ; 32
        PREPARE_VIDEO_7         ; 39
        jmp     video_sub       ; 42=>68

no_vid19:
        WASTE_5                 ; 23
        ____SPKR_DUTY____4      ; 27
ad19b:  ldx     $A8FF           ; 31
        WASTE_28                ; 59
        STORE_JUMP_TGT_3        ; 62
        JUMP_NEXT_6             ; 68
