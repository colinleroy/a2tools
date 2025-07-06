; Duty cycle 30 must toggle off speaker at cycle 38, but we would have to jump
; to video_sub at cycle 39, so this one uses different entry points to
; the video handler to fix this.
; -----------------------------------------------------------------
duty_cycle30:                    ; end spkr at 38
        ____SPKR_DUTY____4      ; 4
ad30:   ldx     $A8FF           ; 8
vs30:   lda     $99FF           ; 12
        and     has_byte        ; 15
        beq     no_vid30        ; 17/18
vd30:   ldy     $98FF           ; 21
        WASTE_3                 ; 24
        PREPARE_VIDEO_7         ; 31
        jmp     video_spkr_sub  ; 34=>68

no_vid30:
ad30b:  ldx     $A8FF           ; 22
        WASTE_12                ; 34
        ____SPKR_DUTY____4      ; 38
        WASTE_21                ; 59
        STORE_JUMP_TGT_3        ; 62
        JUMP_NEXT_6             ; 68
