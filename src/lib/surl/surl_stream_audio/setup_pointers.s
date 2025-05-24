; --------------------------------------
setup_pointers:
        ; Setup vumeter X/Y
        sta     CV
        jsr     VTABZ
        jsr     popa
        sta     CH
        tay
        pha
        lda     #'('|$80
        sta     (BASL),y
        tya
        clc
        adc     #(MAX_LEVEL+2)
        sta     CH
        tay
        lda     #')'|$80
        sta     (BASL),y
        pla
        clc
        adc     #1
        adc     BASL
        sta     vumeter_base
        lda     BASH
        adc     #0
        sta     vumeter_base+1

        ; Setup title X/Y
        jsr     popa
        sta     CV
        jsr     VTABZ
        lda     BASL
        sta     title_addr
        lda     BASH
        sta     title_addr+1

        ; Setup numcols
        jsr     popa
        sta     numcols

        ; Setup pointer access to SPKR
        lda     #<(SPKR)
        sta     spkr_ptr
        lda     #>(SPKR)
        sta     spkr_ptr+1

        ; Setup a space for vumeter
        lda     #' '|$80
        sta     tmp2

        lda     #0
        sta     cancelled

        ; Setup zero-page jump
        lda     #$4C            ; JMP
        sta     zp_jmp
        lda     #$00
        sta     zp_jmp+1
        lda     #$00
        sta     zp_jmp+2
        rts
