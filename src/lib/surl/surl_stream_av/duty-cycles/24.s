duty_cycle24:                    ; end spkr at 32
        ____SPKR_DUTY____4      ; 4
ad24:   ldx     $A8FF           ; 8
vs24:   lda     $99FF           ; 12
        and     #HAS_BYTE       ; 14
        beq     no_vid24        ; 16/17
vd24:   ldy     $98FF           ; 20
        WASTE_8                 ; 28
        ____SPKR_DUTY____4      ; 32
        WASTE_3                 ; 35
        tya                     ; 37
        cpy     #PAGE_TOGGLE    ; 39
        jmp     video_sub       ; 42=>68

no_vid24:
ad24b:  ldx     $A8FF           ; 21
        WASTE_7                 ; 28
        ____SPKR_DUTY____4      ; 32
        WASTE_27                ; 59
        JUMP_NEXT_9             ; 68
