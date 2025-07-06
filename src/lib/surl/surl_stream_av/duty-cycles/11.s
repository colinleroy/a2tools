duty_cycle11:                    ; end spkr at 19
        ____SPKR_DUTY____4      ; 4
ad11:   ldx     $A8FF           ; 8
vs11:   lda     $99FF           ; 12
        WASTE_3                 ; 15
        ____SPKR_DUTY____4      ; 19
        and     has_byte        ; 22
        beq     no_vid11        ; 24/25
vd11:   ldy     $98FF           ; 28
        WASTE_4                 ; 32
        PREPARE_VIDEO_7         ; 39
        jmp     video_sub       ; 42=>68

no_vid11:
ad11b:  ldx     $A8FF           ; 29
        WASTE_30                ; 59
        STORE_JUMP_TGT_3        ; 62
        JUMP_NEXT_6             ; 68
