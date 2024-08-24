duty_cycle15:                    ; end spkr at 23
        ____SPKR_DUTY____4      ; 4
ad15:   ldx     $A8FF           ; 8
vs15:   lda     $99FF           ; 12
        and     #HAS_BYTE       ; 14
        beq     no_vid15        ; 16/17
        WASTE_3                 ; 19
        ____SPKR_DUTY____4      ; 23
vd15:   ldy     $98FF           ; 27
        WASTE_5                 ; 32
        PREPARE_VIDEO_7         ; 39
        jmp     video_sub       ; 42=>68

no_vid15:
        WASTE_2                 ; 19
        ____SPKR_DUTY____4      ; 23
ad15b:  ldx     $A8FF           ; 27
        lda     kbd_cmd         ; 30     handle subtitles switch
        ldy     cur_mix         ; 33
        cmp     #$09            ; 35
        bne     not_tab         ; 37/38
        lda     $C052,y         ; 41     not BIT, to preserve V flag
        tya                     ; 43
        eor     #$01            ; 45
        sta     cur_mix         ; 48
        lda     #$00            ; 50     clear kbd_cmd
        sta     kbd_cmd         ; 53
        WASTE_3                 ; 56
        JUMP_NEXT_12            ; 68

not_tab:
        WASTE_18                ; 56
        JUMP_NEXT_12            ; 68
