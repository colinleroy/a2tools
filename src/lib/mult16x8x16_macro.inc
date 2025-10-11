
        .importzp       tmp1, tmp2, ptr1, ptr2

        .import         popax

;
; 16 bit x 8 bit unsigned multiply, 16 bit result, no overflow indication
; Average cycles: 114

.macro MULT_16x8r16 OUTLABEL
.scope
         tay                  ; Get inputA low byte back at BhAl

AlBl:
         ldx     inputB
         stx     resH         ; init result. If inputB is zero we need to
         stx     resL         ; do it before skipping. Otherwise they'll
         beq     BhAl         ; be overwritten by this first multiplication.
         MULT_AX_STORE_LOW resL
         sta     resH

BhAl:
         tya                  ; Get back inputA low byte
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
