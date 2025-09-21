        .export       _copy_data
        .import       _row_idx, _row_idx_plus2

        .importzp     tmp1
        .importzp     _zp2, _zp4, _zp6, _zp7, _zp13

WIDTH = 640

_raw_ptr1 = _zp2
_cur_buf_0l = _zp4
_cur_buf_0h = _zp6
_col        = _zp7
_rep        = _zp13

.proc _copy_data
        clc
        ldx     _row_idx+1
        ldy     _row_idx
        bne     nouf19
        dex
nouf19:
        dey
        sty     _raw_ptr1
        stx     _raw_ptr1+1

        ldy     #4
        sty     tmp1
y_loop:

        ldx     #4
        lda     tmp1
        and     #1
        beq     even_y
        lda     #<((WIDTH/4)-2)
        sta     end_copy_loop+1
        ldy     #0
        sty     start_copy_loop+1
        jmp     copy_loop
even_y:
        lda     #<((WIDTH/4)-1)
        sta     end_copy_loop+1
        ldy     #1
        sty     start_copy_loop+1
start_copy_loop:
        ldy     #$F0
copy_loop:
        lda     (_raw_ptr1),y
        iny
        sta     (_raw_ptr1),y
        iny
end_copy_loop:
        cpy     #$F1
        bne     copy_loop

        clc
        lda     _raw_ptr1
        adc     #<(WIDTH/4)
        sta     _raw_ptr1
        lda     _raw_ptr1+1
        adc     #>(WIDTH/4)
        sta     _raw_ptr1+1

        dex
        bne     start_copy_loop

        ;  Finish Y loop */
        dec     tmp1
        bne     y_loop

        clc
        lda     _row_idx
        adc     #<(WIDTH<<2)
        sta     _row_idx
        lda     _row_idx+1
        adc     #>(WIDTH<<2)
        sta     _row_idx+1

        lda     _row_idx_plus2
        adc     #<(WIDTH<<2)
        sta     _row_idx_plus2
        lda     _row_idx_plus2+1
        adc     #>(WIDTH<<2)
        sta     _row_idx_plus2+1

        rts
.endproc
