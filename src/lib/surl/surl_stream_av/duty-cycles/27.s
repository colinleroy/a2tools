duty_cycle27:                    ; end spkr at 35
        ____SPKR_DUTY____4      ; 4
ad27:   ldx     $A8FF           ; 8
vs27:   lda     $99FF           ; 12
        and     has_byte        ; 15
        beq     no_vid27        ; 17/18
vd27:   ldy     $98FF           ; 21
        WASTE_3                 ; 24
        PREPARE_VIDEO_7         ; 31
        ____SPKR_DUTY____4      ; 35
        WASTE_4                 ; 39
        jmp     video_sub       ; 42=>68

no_vid27:
ad27b:  ldx     $A8FF           ; 22
        WASTE_9                 ; 31
        ____SPKR_DUTY____4      ; 35
        WASTE_24                ; 59
        STORE_JUMP_TGT_3        ; 62
        JUMP_NEXT_6             ; 68
