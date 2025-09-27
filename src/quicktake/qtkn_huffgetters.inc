
.macro CTRL_REFILLER filler_addr,rts_addr
filler_addr:
        jsr     _bitbuf_refill
        jmp     rts_addr
.endmacro

; Returns value in A
.macro GETCTRLHUFF filler_addr,rts_addr
        lda    #0             ; r = 0
        tay                   ; n = 0
        ldx     _vbits

read_bit:
        iny                   ; Read until valid code
        dex
        bmi     filler_addr
rts_addr:
        asl     _bitbuf
        rol     a
        sta     bitscheckc+1    ; Patch bitcheck address
                              ; cpy $nnnn,x is impossible so this is faster

bitscheckc:
huff_numc = *+2             ; Get num bits
        cpy     _huff_ctrl
        bne     read_bit
        stx     _vbits

        tax
huff_numc_h = *+2
        lda     _huff_ctrl+256,x
.endmacro
