duty_cycle17:                    ; end spkr at 25
        ____SPKR_DUTY____4      ; 4
ad17:   ldx     $A8FF           ; 8
vs17:   lda     $99FF           ; 12
        and     #HAS_BYTE       ; 14
        beq     no_vid17        ; 16/17
        WASTE_5                 ; 21
        ____SPKR_DUTY____4      ; 25
vd17:   ldy     $98FF           ; 29
        WASTE_3                 ; 32
        PREPARE_VIDEO_7         ; 39
        jmp     video_sub       ; 42=>68

no_vid17:
ad17b:  ldx     $A8FF           ; 21
        ____SPKR_DUTY____4      ; 25

        ; Handle mono HGR depending on subtitles
        ; Because we can't have both.
        lda hgr_mono_file       ; 29
        beq out_sub1            ; 31 / 32
        ldy cur_mix             ; 34
        beq out_sub2            ; 36 / 37
bw:
        lda DHIRESON            ; 40
        jmp out_bw              ; 43
out_sub1:
        WASTE_5                 ; 37
out_sub2:
        lda DHIRESOFF           ; 41
        WASTE_2                 ; 43
out_bw:
        WASTE_16                ; 59
        JUMP_NEXT_9             ; 68
