duty_cycle12:                    ; end spkr at 20
        ____SPKR_DUTY____4      ; 4
ad12:   ldx     $A8FF           ; 8
vs12:   lda     $99FF           ; 12
        WASTE_4                 ; 16
        ____SPKR_DUTY____4      ; 20
        and     has_byte        ; 23
        beq     no_vid12        ; 25/26
vd12:   ldy     $98FF           ; 29
        WASTE_3                 ; 32
        PREPARE_VIDEO_7         ; 39
        jmp     video_sub       ; 42=>68

no_vid12:
ad12b:  ldx     $A8FF           ; 30
        WASTE_29                ; 59
        STORE_JUMP_TGT_3        ; 62
        JUMP_NEXT_6             ; 68
