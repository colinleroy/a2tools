
        .importzp       tmp1, tmp2, ptr1, ptr2

        .import         popax
        .export         _mult16x16x16, mult16x16x16_direct

        .include        "mult8x8x16_macro.inc"
;
; 16 bit x 16 bit unsigned multiply, 16 bit result, no overflow indication
; Average cycles: 114

inputA = ptr1
inputB = ptr2
resL   = tmp1
resH   = tmp2
;  inputA+1          inputA
;* inputB+1          inputB
;______
;   inputB*inputA
; + inputB*(inputA+1) << 8
; + (inputB+1)*inputA << 8
 _mult16x16x16:
         sta     inputB
         stx     inputB+1
         jsr     popax

; Multiply inputB(ptr2) by AX (X:h, A:L), result in AX (X:h, A:L)
mult16x16x16_direct:
         ldy     #$00         ; init result to zero
         sty     resH         ; costs 8 cycles but the shortcuts
         sty     resL         ; are worth it

         stx     inputA+1
         sta     inputA

AlBl:
         ldx     inputB
         beq     BhAl        ; Skip both 8x8 mult with inputB low if 0
         MULT_AX_STORE_LOW resL
         sta     resH

         ; X is still inputB
AhBl:
         lda     inputA+1
         beq     BhAl         ; Skip if inputA low is 0
         MULT_AX_NO_HIGH
         clc
         adc     resH
         sta     resH

BhAl:
         lda     inputA
         beq     skipLast
         ldx     inputB+1
         beq     skipLast
         MULT_AX_NO_HIGH
         clc
         adc     resH
         tax
         lda     resL
         rts
skipLast:
         ldx     resH
         lda     resL
         rts
