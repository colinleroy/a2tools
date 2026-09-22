        .export _mul_362, _mul_473, _mul_277, _mul_669

        .import _mul362_h, _mul362_m, _mul362_l
        .import _mul473_h, _mul473_m, _mul473_l
        .import _mul277_h, _mul277_m, _mul277_l
        .import _mul669_h, _mul669_m, _mul669_l

        .segment "CODE"

; int8 * x => >> 8 => (int8)
.macro do_mul TABL, TABM;, TABH
.scope
        bmi     neg
        tay
        lda     TABM,y
        ; ldx     TABH,y
        rts

neg:    clc
        eor     #$FF
        adc     #1
        tax

        clc
        lda     TABL,x
        eor     #$FF
        adc     #1
        lda     TABM,x
        eor     #$FF
        adc     #0
        ; tay
        ; lda     TABH,x
        ; eor     #$FF
        ; adc     #0
        ; tax
        ; tya
        rts
.endscope
.endmacro

.proc _mul_362
        do_mul _mul362_l, _mul362_m ;, _mul_362_h
.endproc

.proc _mul_473
        do_mul _mul473_l, _mul473_m ;, _mul_473_h
.endproc

.proc _mul_277
        do_mul _mul277_l, _mul277_m; , _mul_277_h
.endproc

.proc _mul_669
        do_mul _mul669_l, _mul669_m;, _mul_669_h
.endproc
