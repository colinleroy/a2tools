
        .importzp       tmp1, tmp2, ptr1, ptr2

        .import         popax

        .include        "mult8x8x16_macro.inc"
;
; 16 bit x 16 bit unsigned multiply, 16 bit result, no overflow indication
; Average cycles: 114

inputB = ptr2
resL   = tmp1
resH   = tmp2

.macro MULT_16x16r16 OUTLABEL
.scope
         stx     AhBl+1
         tay                  ; Get back at BhAl

AlBl:
         ldx     inputB
         stx     resH         ; init result. If inputB is zero we need to
         stx     resL         ; do it before skipping. Otherwise they'll
         beq     BhAl         ; be overwritten by this first multiplication.
         MULT_AX_STORE_LOW resL
         sta     resH

         ; X is still inputB
AhBl:
         lda     #$FF         ; Patched with A high byte
         beq     BhAl         ; Skip if inputA high is 0
         MULT_AX_NO_HIGH
         clc
         adc     resH
         sta     resH

BhAl:
         tya                  ; Get back inputA low byte
         beq     skipLast
         ldx     inputB+1
         beq     skipLast
         MULT_AX_NO_HIGH
         clc
         adc     resH
         tax
         lda     resL
         .ifblank OUTLABEL
         rts
         .else
         jmp     OUTLABEL
         .endif
skipLast:
         ldx     resH
         lda     resL
.endscope
.endmacro
