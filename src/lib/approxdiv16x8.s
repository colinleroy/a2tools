
        .importzp       tmp4, ptr2, sreg

        .import         popax, sqrLow, sqrHigh, invLow, invHigh, popptr1
        .import         _mult16x16x32, mult16x16x32_direct
        .export         approx_div16x8_direct, _approx_div16x8

_approx_div16x8:
        sta     tmp4
        jsr     popax
        ldy     tmp4

approx_div16x8_direct:
        sta     ptr2
        stx     ptr2+1
        ldx     invHigh,y
        lda     invLow,y
        jsr     mult16x16x32_direct
        ldx     sreg+1
        lda     sreg
        rts
