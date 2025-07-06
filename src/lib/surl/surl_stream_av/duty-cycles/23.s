duty_cycle23:                    ; end spkr at 31
        ____SPKR_DUTY____4      ; 4
ad23:   ldx     $A8FF           ; 8
vs23:   lda     $99FF           ; 12
        and     has_byte        ; 15
        beq     no_vid23        ; 17/18
vd23:   ldy     $98FF           ; 21
        STORE_JUMP_TGT_3        ; 24
        WASTE_3                 ; 27
        ____SPKR_DUTY____4      ; 31
        PREPARE_VIDEO_E4        ; 35
        WASTE_4                 ; 39
        jmp     video_sub       ; 42=>68

no_vid23:
ad23b:  ldx     $A8FF           ; 22
        WASTE_5                 ; 27
        ____SPKR_DUTY____4      ; 31
        WASTE_28                ; 59
        STORE_JUMP_TGT_3        ; 62
        JUMP_NEXT_6             ; 68
