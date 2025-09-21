        .export       _copy_data, _consume_extra
        .import       _row_idx, _row_idx_plus2

        .import       _getdatahuff, _getdatahuff8
        .import       _getdatahuff_refilled, _getdatahuff8_refilled
        .import       _getctrlhuff
        .import       _getctrlhuff_refilled
        .import       _refill_ret

        .import       _huff_numc, _huff_numc_h
        .import       _huff_numd, _huff_numd_h

        .import       _huff_data, _huff_ctrl

        .importzp     tmp1
        .importzp     _zp2, _zp4, _zp6, _zp7, _zp13

WIDTH = 640

_raw_ptr1 = _zp2
_cur_buf_0l = _zp4
_cur_buf_0h = _zp6
_col        = _zp7
_rep        = _zp13


.macro SET_REFILL_RET addr
        ldx     #<addr
        stx     _refill_ret+1
        ldx     #>addr
        stx     _refill_ret+2
.endmacro

.proc _copy_data
        clc
        ldx     _row_idx+1
        ldy     _row_idx
        bne     :+
        dex
:
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

.proc _consume_extra
        lda     #2
        sta     _c
c_loop:
        lda     #1
        sta     _tree
        lda     #<(WIDTH/4)
        sta     _col

col_loop2:
        lda     _tree
        asl
        adc     #>_huff_ctrl
        sta     _huff_numc
        adc     #1
        sta     _huff_numc_h
        SET_REFILL_RET _getctrlhuff_refilled

        jsr     _getctrlhuff
        sta     _tree

        beq     tree_zero_2

tree_not_zero_2:
        dec     _col

        ;huff_ptr = huff[tree + 10]
        cmp     #8
        bne     norm_huff
        SET_REFILL_RET _getdatahuff8_refilled

        jsr     _getdatahuff8
        jsr     _getdatahuff8
        jsr     _getdatahuff8
        jsr     _getdatahuff8
        jmp     tree_zero_2_done

norm_huff:
        adc     #>(_huff_data+256)
        sta     _huff_numd
        sta     _huff_numd_h
        SET_REFILL_RET _getdatahuff_refilled

        jsr     _getdatahuff
        jsr     _getdatahuff
        jsr     _getdatahuff
        jsr     _getdatahuff

        jmp     tree_zero_2_done

tree_zero_2:
        lda     _col
        cmp     #2
        bcs     col_gt1

        lda     #1
        jmp     check_nreps_2

col_gt1:
        ;data tree 0
        ldx     #>_huff_data
        stx     _huff_numd
        stx     _huff_numd_h
        SET_REFILL_RET _getdatahuff_refilled

        jsr     _getdatahuff
        clc
        adc     #1

check_nreps_2:
        sta     _nreps

        cmp     #9
        bcc     nreps_check_done_2
        lda     #8
nreps_check_done_2:
        sta     _rep_loop

        ;data tree 1
        ldx     #>(_huff_data+256)
        stx     _huff_numd
        stx     _huff_numd_h
        ; refiller return already set

        ldx     #$00
        stx     _rep

do_rep_loop_2:
        dec     _col

        txa
        and     #1
        beq     rep_even_2

        jsr     _getdatahuff

rep_even_2:
        ldx     _rep
        inx
        cpx     _rep_loop
        beq     rep_loop_2_done
        stx     _rep
        lda     _col
        bne     do_rep_loop_2
        beq     check_c_loop
rep_loop_2_done:
        lda     _nreps
        cmp     #9
        beq     tree_zero_2

tree_zero_2_done:
        lda     _col
        beq     check_c_loop
        jmp     col_loop2

check_c_loop:
        dec     _c
        beq     c_loop_done
        jmp     c_loop
c_loop_done:
        rts
.endproc

.bss

_c:             .res 1
_tree:          .res 1
_rep_loop:      .res 1
_nreps:         .res 1
