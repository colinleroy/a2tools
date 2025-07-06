duty_cycle17:                    ; end spkr at 25
        ____SPKR_DUTY____4      ; 4
ad17:   ldx     $A8FF           ; 8
vs17:   lda     $99FF           ; 12
        and     has_byte        ; 15
        bne     vid17           ; 17/18 WARNING! inverted

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
        STORE_JUMP_TGT_3        ; 62
        JUMP_NEXT_6             ; 68

vid17:
        WASTE_3                 ; 21
        ____SPKR_DUTY____4      ; 25
vd17:   ldy     $98FF           ; 29
        WASTE_3                 ; 32
        PREPARE_VIDEO_7         ; 39
        jmp     video_sub       ; 42=>68
