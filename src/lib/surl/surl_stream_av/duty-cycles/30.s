; Duty cycle 30 must toggle off speaker at cycle 38, but we would have to jump
; to video_sub at cycle 39, so this one uses different entry points to
; the video handler to fix this.
; -----------------------------------------------------------------
duty_cycle30:                    ; end spkr at 38
        ____SPKR_DUTY____4      ; 4
ad30:   ldx     $A8FF           ; 8
vs30:   lda     $99FF           ; 12
        and     #HAS_BYTE       ; 14
        beq     no_vid30        ; 16/17
vd30:   ldy     $98FF           ; 20
        WASTE_4                 ; 24
        PREPARE_VIDEO_7         ; 31
        jmp     video_spkr_sub  ; 34=>68

no_vid30:
ad30b:  ldx     $A8FF           ; 21
        WASTE_13                ; 34
        ____SPKR_DUTY____4      ; 38
        WASTE_18                ; 56
        JUMP_NEXT_12            ; 68
