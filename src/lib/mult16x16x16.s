
        .importzp       tmp1, tmp2, ptr1, ptr2

        .import         popax
        .export         _mult16x16x16, mult16x16x16_direct

        .include    "../lib/mult8x8x16_macro.inc"
        .include    "../lib/mult16x16x16_macro.inc"
;
; 16 bit x 16 bit unsigned multiply, 16 bit result, no overflow indication
; Average cycles: 114

 _mult16x16x16:
         sta     inputB
         stx     inputB+1
         jsr     popax

; Multiply inputB(ptr2) by AX (X:h, A:L), result in AX (X:h, A:L)
mult16x16x16_direct:
         MULT_16x16r16
         rts
