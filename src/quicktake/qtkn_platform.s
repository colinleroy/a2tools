        .export       _copy_data
        .export       _consume_extra
        .export       _init_row

        .import       _row_idx, _row_idx_plus2

        .import       _getdatahuff, _getdatahuff8
        .import       _getdatahuff_refilled, _getdatahuff8_refilled
        .import       _getctrlhuff
        .import       _getctrlhuff_refilled
        .import       _refill_ret

        .import       _huff_numc, _huff_numc_h
        .import       _huff_numd, _huff_numd_h

        .import       _huff_data, _huff_ctrl
        .import       _factor, _last
        .import       _val_from_last, _val_hi_from_last
        .import       _buf_0

        .import       mult16x16r24_direct, mult8x8r16_direct
        .import       tosmula0, pushax
        .importzp     tmp1, sreg, ptr2
        .importzp     _zp2, _zp4, _zp6, _zp7, _zp13

WIDTH               = 640
USEFUL_DATABUF_SIZE = 321
DATABUF_SIZE        = $400

raw_ptr1    = _zp2
wordcnt     = _zp2

cur_buf_0l = _zp4
cur_buf_0h = _zp6
_col       = _zp7
_rep       = _zp13


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
        sty     raw_ptr1
        stx     raw_ptr1+1

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
        lda     (raw_ptr1),y
        iny
        sta     (raw_ptr1),y
        iny
end_copy_loop:
        cpy     #$F1
        bne     copy_loop

        clc
        lda     raw_ptr1
        adc     #<(WIDTH/4)
        sta     raw_ptr1
        lda     raw_ptr1+1
        adc     #>(WIDTH/4)
        sta     raw_ptr1+1

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

.proc _init_row
        lda     #<(_buf_0+256)
        ldx     #>(_buf_0+256)
        sta     cur_buf_0l
        stx     cur_buf_0l+1
        lda     #<(_buf_0+256+512)
        ldx     #>(_buf_0+256+512)
        sta     cur_buf_0h
        stx     cur_buf_0h+1

        ldy     _last
        cpy     #18
        bcs     small_val

        lda     _val_from_last,y
        ldx     _val_hi_from_last,y
        jsr     pushax
        lda     _factor
        sta     _last
        jsr     tosmula0
        jmp     shift_val

small_val:
        lda     _val_from_last,y
        ldx     _factor
        stx     _last
        jsr     mult8x8r16_direct
shift_val:
        stx     ptr2+1
        lsr     ptr2+1
        ror     a
        lsr     ptr2+1
        ror     a
        lsr     ptr2+1
        ror     a
        lsr     ptr2+1
        ldx     ptr2+1
        ror     a
        sta     ptr2
        cmp     #$FF
        bne     check0x100
        cpx     #$00
        bne     check0x100

mult_FF:
        ldy     #<USEFUL_DATABUF_SIZE
        sty     wordcnt
        lda     #>USEFUL_DATABUF_SIZE
        sta     wordcnt+1

setup_curbuf_x_ff:
        ; load
        dey
        sty     wordcnt
        lda     (cur_buf_0h),y
        tax
        bne     not_null_buf_ff
        lda     (cur_buf_0l),y
        beq     null_buf_ff

not_null_buf_ff:
        lda     (cur_buf_0l),y
        ; tmp32 in AX
        stx     tmp1
        sec
        sbc     tmp1
        tay
        txa
        sbc     #0
        tax
        tya
        jmp     store_buf_ff

null_buf_ff:
        lda     #$0F
        ldx     #$00

store_buf_ff:
        ldy     wordcnt
        sta     (cur_buf_0l),y
        txa
        sta     (cur_buf_0h),y

        ldy     wordcnt
        bne     setup_curbuf_x_ff
        dec     cur_buf_0l+1
        dec     cur_buf_0h+1
        dec     wordcnt+1
        bpl     setup_curbuf_x_ff
        jmp     init_done

check0x100:
        cpx     #$01
        bne     slow_mults
        cmp     #$00
        beq     init_done ; nothing to do!

slow_mults:
        ldy     #<USEFUL_DATABUF_SIZE
        sty     wordcnt
        lda     #>USEFUL_DATABUF_SIZE
        sta     wordcnt+1

setup_curbuf_x_slow:
        ; load
        dey
        sty     wordcnt
        lda     (cur_buf_0h),y
        tax
        bne     not_null_buf
        lda     (cur_buf_0l),y
        beq     null_buf

not_null_buf:
        lda     (cur_buf_0l),y

        ; multiply
        jsr     mult16x16r24_direct

        ; Shift >> 8
        txa
        jmp     store_buf

        null_buf:
        lda     #$0F
        ldx     #$00
        stx     sreg

        store_buf:
        ldy     wordcnt
        sta     (cur_buf_0l),y
        lda     sreg
        sta     (cur_buf_0h),y

        ldy     wordcnt
        bne     setup_curbuf_x_slow
        dec     cur_buf_0l+1
        dec     cur_buf_0h+1
        dec     wordcnt+1
        bpl     setup_curbuf_x_slow
init_done:
        rts
.endproc

.bss

_c:             .res 1
_tree:          .res 1
_rep_loop:      .res 1
_nreps:         .res 1
