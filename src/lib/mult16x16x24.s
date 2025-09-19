
        .importzp       tmp1, tmp2, ptr1, ptr2, sreg

        .import         popax
        .export         _mult16x16r24, mult16x16r24_direct

        .include        "mult8x8x16_macro.inc"
;
; 16 bit x 16 bit unsigned multiply, 24 bit result, no overflow indication
; Average cycles: 180

inputB = ptr2
resL   = tmp1
resH   = tmp2
;  inputA+1          inputA
;* inputB+1          inputB
;______
;   inputB*inputA
; + inputB*(inputA+1) << 8
; + (inputB+1)*inputA << 8
 _mult16x16r24:
         sta     inputB
         stx     inputB+1
         jsr     popax

; Multiply inputB(ptr2) by AX (X:h, A:L), result in sreg/AX (sreg:h, X:m, A:L)
mult16x16r24_direct:
         stx     AhBl+1
         sta     BhAl+1
         ldy     #0
         sty     sreg

AlBl:
         ldx     inputB
         stx     resH         ; init result. If inputB is zero we need to
         stx     resL         ; do it before skipping. Otherwise they'll
         beq     doBhAl       ; be overwritten by this first multiplication.
         MULT_AX_STORE_LOW resL
         sta     resH

         ; X is still inputB
AhBl:
         lda     #$FF         ; Patched with A high byte
         beq     doBhAl       ; Skip if inputA high is 0
         MULT_AX_STORE_HIGH sreg
         clc
         adc     resH
         sta     resH
         bcc     doBhAl
         inc     sreg

doBhAl:
         ldx     inputB+1
         beq     skipBh       ; Skip last two
BhAl:
         lda     #$FF         ; Patched with A low byte
         beq     BhAh
         MULT_AX_STORE_HIGH BhAlRh+1
         clc
         adc     resH
         sta     resH

BhAlRh:  lda     #$FF
         adc     sreg
         sta     sreg

         ; X still Bh
BhAh:
         lda     AhBl+1
         beq     skipBh
         MULT_AX_NO_HIGH
         clc
         adc     sreg
         sta     sreg
skipBh:
         lda     resL
         ldx     resH
         rts
