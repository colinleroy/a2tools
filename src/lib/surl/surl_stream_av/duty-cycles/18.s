duty_cycle18:                    ; end spkr at 26
        ____SPKR_DUTY____4      ; 4
ad18:   ldx     $A8FF           ; 8
vs18:   lda     $99FF           ; 12
        and     has_byte        ; 15
        beq     no_vid18        ; 17/18
        WASTE_5                 ; 22
        ____SPKR_DUTY____4      ; 26
vd18:   ldy     $98FF           ; 30
        WASTE_2                 ; 32
        PREPARE_VIDEO_7         ; 39
        jmp     video_sub       ; 42=>68

no_vid18:
        WASTE_4                 ; 22
        ____SPKR_DUTY____4      ; 26
ad18b:  ldx     $A8FF           ; 30
        WASTE_29                ; 59
        JUMP_NEXT_9             ; 68
