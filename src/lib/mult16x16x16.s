
        .importzp       tmp1, tmp2, ptr1, ptr2

        .import         popax, sqrLow, sqrHigh, invLow, invHigh, popptr1
        .export         _mult16x16x16, mult16x16x16_direct

; based on the 8 bit multiply of mult57.a (https://sites.google.com/site/h2obsession/programming/6502)
; then adjusted for 16 bit multiplication by TobyLobster 
; (https://github.com/TobyLobster/multiply_test/blob/main/tests/mult56.a)
;
; 16 bit x 16 bit unsigned multiply, 16 bit result
; Average cycles: 167
; 1210 bytes

inputA = ptr1
inputB = ptr2

; ***************************************************************************************
; 16 bit x 16 bit unsigned multiply, 16 bit result
;
; On Entry:
;   inputA: multiplier   (2 bytes)
;   inputB: multiplicand (2 bytes)
;
; On Exit:
;   result: product (2 bytes in AX)

_mult16x16x16:
        sta     inputB
        stx     inputB+1
        jsr     popax
        sta     inputA
        stx     inputA+1

        ldx     inputB          ; (a0*b0)

; Expects low byte of ptr2 in X, low byte of ptr1 in A
mult16x16x16_direct:
        sta     getLow1+1
        sta     getHigh1+1
        sec
        sbc     inputB
        bcs     :+
        sbc     #0
        eor     #255

:       tay
getLow1:
        lda     sqrLow,x        ; Patched
        sbc     sqrLow,y
        sta     sta_result+1    ; low byte
getHigh1:
        lda     sqrHigh,x       ; Patched
        sbc     sqrHigh,y
        sta     sta_tmp_result_1a+1        ; high byte

        lda     inputA+1        ; (a1*b0)
        ldx     inputB
        sta     getLow2+1
        sta     getHigh2+1
        sec
        sbc     inputB
        bcs     :+
        sbc     #0
        eor     #255

:       tay
getLow2:
        lda     sqrLow,x        ; Patched
        sbc     sqrLow,y
        sta     temp1+1
getHigh2:
        lda     sqrHigh,x       ; Patched
        sbc     sqrHigh,y
        tax
temp1:
        lda     #0              ; Patched

        clc
sta_tmp_result_1a:
        adc     #$FF
        sta     sta_tmp_result_1b+1

        lda     inputA          ;(a0*b1)
        ldx     inputB+1
        sta     getLow3+1
        sta     getHigh3+1
        sec
        sbc     inputB+1
        bcs     :+
        sbc     #0
        eor     #255

:       tay
getLow3:
        lda     sqrLow,x        ; Patched
        sbc     sqrLow,y
        sta     temp2+1
getHigh3:
        lda     sqrHigh,x       ; Patched
        sbc     sqrHigh,y
        tax
temp2:
        lda     #0              ; Patched

        clc
sta_tmp_result_1b:
        adc     #$FF
        tax
sta_result:
        lda     #$FF
        rts
