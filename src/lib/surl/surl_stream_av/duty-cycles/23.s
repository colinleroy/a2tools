duty_cycle23:                    ; end spkr at 31
        ____SPKR_DUTY____4      ; 4
ad23:   ldx     $A8FF           ; 8
vs23:   lda     $99FF           ; 12
        and     #HAS_BYTE       ; 14
        beq     no_vid23        ; 16/17
vd23:   ldy     $98FF           ; 20
        PREPARE_VIDEO_7         ; 27
        ____SPKR_DUTY____4      ; 31
        WASTE_8                 ; 39
        jmp     video_sub       ; 42=>68

no_vid23:
ad23b:  ldx     $A8FF           ; 21
        WASTE_6                 ; 27
        ____SPKR_DUTY____4      ; 31
        WASTE_28                ; 59
        JUMP_NEXT_9             ; 68
