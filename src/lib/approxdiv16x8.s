
        .importzp       tmp2, tmp3, tmp4
        .import         invLow, invHigh, popax
        .export         approx_div16x8_direct
        .export         _approx_div16x8
        .include        "mult8x8x16_macro.inc"

ac_l = tmp2
ac_h = tmp3
ad_h = tmp4

shortcut_first_mults:
        stx     ac_l          ; We need to init the result to zero.
        stx     ac_h
        stx     ad_h
        jmp     last_mult

out:    rts

_approx_div16x8:
        sta     tmp4
        jsr     popax
        ldy     tmp4

; Divide AX (X:h, A:l) by Y
approx_div16x8_direct:
        cpy     #2            ; Do nothing if divisor is 0(!!) or 1
        bcc     out
        cpx     #0
        beq     shortcut_first_mults ; if dividend high byte if 0 skip A*C
        sta     B+1
        txa                          ; we need dividend low byte in A
        sta     A2+1

A1:     ; A already good
C1:     ldx     invHigh,y
        MULT_AX_STORE_LOW ac_l
        sta     ac_h

A2:     lda     #$FF
        ldx     invLow,y
        MULT_AX_STORE_LOW ; No dest for low, we don't care
        sta     ad_h

B:      lda     #$FF      ; Compute BC

; --- back from first mults shortcuts ---
last_mult:
        ldx     invHigh,y
        MULT_AX_STORE_LOW ; No dest for low, we don't care

        ; bc_sum = ad(high only)+bc (in A)
        clc
        adc     ad_h
        ldx     ac_h
        bcc     :+
        inx
:       adc     ac_l
        bcc     :+
        inx
:       rts
