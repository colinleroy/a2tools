duty_cycle15:                    ; end spkr at 23
        ____SPKR_DUTY____4      ; 4
ad15:   ldx     $A8FF           ; 8
vs15:   lda     $99FF           ; 12
        and     has_byte        ; 15
        WASTE_4                 ; 19
        ____SPKR_DUTY____4      ; 23
        beq     no_vid15        ; 25/26
vd15:   ldy     $98FF           ; 29
        WASTE_3                 ; 32
        PREPARE_VIDEO_7         ; 39
        jmp     video_sub       ; 42=>68

no_vid15:
        WASTE_2                 ; 28
ad15b:  ldx     $A8FF           ; 32
        lda     kbd_cmd         ; 35     handle subtitles switch
        ldy     cur_mix         ; 38
        cmp     #$09            ; 40
        bne     not_tab         ; 42/43
        lda     $C052,y         ; 46     not BIT, to preserve V flag
        tya                     ; 48
        eor     #$01            ; 50
        sta     cur_mix         ; 53
        lda     #$00            ; 55     clear kbd_cmd
        sta     kbd_cmd         ; 59
        STORE_JUMP_TGT_3        ; 62
        JUMP_NEXT_6             ; 68

not_tab:
        WASTE_16                ; 59
        STORE_JUMP_TGT_3        ; 62
        JUMP_NEXT_6             ; 68
