duty_cycle28:                    ; end spkr at 36
        ____SPKR_DUTY____4      ; 4
ad28:   ldx     $A8FF           ; 8
vs28:   lda     $99FF           ; 12
        and     has_byte        ; 15
        beq     no_vid28        ; 17/18
vd28:   ldy     $98FF           ; 21
        WASTE_4                 ; 25
        PREPARE_VIDEO_7         ; 32
        ____SPKR_DUTY____4      ; 36
        WASTE_3                 ; 39
        jmp     video_sub       ; 42=>68

no_vid28:
ad28b:  ldx     $A8FF           ; 22
        WASTE_10                ; 32
        ____SPKR_DUTY____4      ; 36
        WASTE_23                ; 59
        STORE_JUMP_TGT_3        ; 62
        JUMP_NEXT_6             ; 68
