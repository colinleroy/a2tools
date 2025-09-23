
        .importzp       tmp1, tmp2, ptr1, ptr2, sreg

        .import         popax
        .export         _mult16x16mid16, mult16x16mid16_direct

        .include        "mult8x8x16_macro.inc"
;
; 16 bit x 16 bit unsigned multiply, 16 bit result of the two middle bytes.
; Quite specialized, this is for the Quicktake 150 algorithm which does a
; uint16 = (uint16*uint16) >> 8.

; Average cycles: 180
inputA = ptr1
inputB = ptr2
res    = tmp1

;  inputA+1          inputA
;* inputB+1          inputB
;______
;   inputB*inputA
; + inputB*(inputA+1) << 8
; + (inputB+1)*inputA << 8
 _mult16x16mid16:
         sta     inputB
         stx     inputB+1
         jsr     popax

; Multiply inputB(ptr2) by AX (X:h, A:L), result in sreg/AX (sreg:h, X:m, A:L)
mult16x16mid16_direct:
         stx     inputA+1
         sta     inputA
         ldy     #0
         sty     res
         sty     res+1
AlBl:
         ldx     inputB
         beq     doBhAl
         MULT_AX_STORE_LOW    ; (don't store low byte)
         sta     res

         ; X is still inputB
AhBl:
         lda     inputA+1
         beq     doBhAl       ; Skip if inputA high is 0
         MULT_AX_STORE_HIGH res+1
         clc
         adc     res
         sta     res
         bcc     doBhAl
         inc     res+1

doBhAl:
         ldx     inputB+1
         beq     loadResult   ; Skip last two
BhAl:
         lda     inputA
         beq     BhAh
         MULT_AX
         clc
         adc     res
         sta     res

         txa
         adc     res+1
         sta     res+1

         ; X destroyed now
         ldx     inputB+1
BhAh:
         lda     inputA+1       ; A high byte
         beq     loadResult
         MULT_AX_NO_HIGH
         clc
         adc     res+1
         tax
         lda     res
         rts

loadResult:
         ldx     res+1
         lda     res
         rts
