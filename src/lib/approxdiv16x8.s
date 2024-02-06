
        .importzp       tmp1, tmp2, ptr1, ptr2, sreg

        .import         popax, sqrLow, sqrHigh, invLow, invHigh, popptr1
        .import         _mult16x16x32, mult16x16x32_direct
        .export         approx_div16x8_direct, _approx_div16x8

_approx_div16x8:
        pha
        jsr     popax
        sta     ptr2
        stx     ptr2+1
        tax
        pla
        tay

; Expects low byte of first operand (ptr2) in X, divisor in A
approx_div16x8_direct:
        lda     invHigh,y
        sta     ptr1+1
        lda     invLow,y
        sta     ptr1
        jsr     mult16x16x32_direct
        ldx     sreg+1
        lda     sreg
        rts
