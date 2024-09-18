duty_cycle16:                    ; end spkr at 24
        ____SPKR_DUTY____4      ; 4
ad16:   ldx     $A8FF           ; 8
vs16:   lda     $99FF           ; 12
        and     #HAS_BYTE       ; 14
        beq     no_vid16        ; 16/17
vd16:   ldy     $98FF           ; 20
        ____SPKR_DUTY____4      ; 24
        WASTE_8                 ; 32
        PREPARE_VIDEO_7         ; 39
        jmp     video_sub       ; 42=>68

no_vid16:
        WASTE_3                 ; 20
        ____SPKR_DUTY____4      ; 24
asp:    lda     $FFFF           ; 28     check serial tx empty
        and     #$10            ; 30
        beq     noser           ; 32/33

        lda     KBD             ; 36     read keyboard
        bpl     nokbd           ; 38/39
        sta     KBDSTRB         ; 42     clear keystrobe
        and     #$7F            ; 44
adp:    sta     $FFFF           ; 48     send cmd
        cmp     #$1B            ; 50     escape?
        beq     out             ; 52/53  if escape, exit forcefully
        sta kbd_cmd             ; 55
        WASTE_4                 ; 59
        JUMP_NEXT_9             ; 68     jump to next duty cycle
nokbd:
ad16b:  ldx     $A8FF           ; 43
        WASTE_16                ; 59
        JUMP_NEXT_9             ; 68
noser:  
        WASTE_26                ; 59
        JUMP_NEXT_9             ; 68

out:    lda     #1              ; 55
        sta     cancelled       ; 59
        JUMP_NEXT_9             ; 68
