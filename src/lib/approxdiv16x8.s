
        .importzp       tmp2, tmp3, tmp4
        .import         invLow, invHigh, popax
        .export         approx_div16x8_direct
        .export         _approx_div16x8
        .include        "mult8x8x16_macro.inc"

ac_l = tmp2
ac_h = tmp3
ad_h = tmp4

shortcut_first_mults:
        ldx     invLow,y
        stx     D+1
        ldx     invHigh,y
        stx     C2+1
        ldy     #$00
        sty     ac_l
        sty     ac_h
        sty     ad_h
        jmp     B

_approx_div16x8:
        sta     tmp4
        jsr     popax
        ldy     tmp4

; Divide AX (X:h, A:l) by Y
approx_div16x8_direct:
        cpy     #2            ; Do nothing if divisor is 0(!!) or 1
        bcs     :+
        rts
:       sta     B+1
        txa     ; for A1
        beq     shortcut_first_mults ; if dividend high byte if 0
        sta     A2+1
        ldx     invLow,y
        stx     D+1
        ldx     invHigh,y
        stx     C2+1

A1:     ; A already good
C1:     ; X already invHigh,y
        MULT_AX_YA
        sty     ac_l
        sta     ac_h

A2:
        lda     #$FF
D:
        ldx     #$FF
        MULT_AX_STORE_LOW ; No dest for low, we don't care
        sta     ad_h

B:      ; Compute BC
        lda     #$FF
C2:
        ldx     #$FF
        MULT_AX_STORE_LOW ; No dest for low, we don't care

        ; bc_sum = ad(high only)+bc (in A)
        clc
        adc     ad_h
        bcc     nocarry
        adc     ac_l
        tay
        lda     ac_h
        adc     #$01
        tax
        tya
        rts

nocarry:
        adc     ac_l
        tay
        lda     ac_h
        adc     #$00
        tax
        tya
        rts
