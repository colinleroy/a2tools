        .export       _copy_data
        .export       _consume_extra
        .export       _init_row
        .export       _decode_row

        .export       _init_top
        .export       _init_shiftl4
        .export       _init_shiftl3
        .export       _init_buf_0
        .export       _init_div48
        .export       _init_huff

        .import       _row_idx, _row_idx_plus2

        .import       _getdatahuff, _getdatahuff8
        .import       _getdatahuff_refilled, _getdatahuff8_refilled
        .import       _getctrlhuff
        .import       _getctrlhuff_refilled
        .import       _refill_ret

        .import       _huff_numc, _huff_numc_h
        .import       _huff_numd, _huff_numd_h

        .import       _huff_data, _huff_ctrl, _src
        .import       _factor, _last
        .import       _val_from_last, _val_hi_from_last
        .import       _buf_0, _buf_1, _buf_2
        .import       _div48_l, _div48_h
        .import       _shiftl3
        .import       _shiftl4n_l, _shiftl4n_h
        .import       _shiftl4p_l, _shiftl4p_h

        .import       mult16x16r24_direct, mult8x8r16_direct
        .import       approx_div16x8_direct
        .import       tosmula0, pushax, pusha0
        .import       _memset, _memcpy, aslax4

        .importzp     tmp1, sreg, ptr2, tmp2, tmp3, tmp4
        .importzp     _zp2, _zp3, _zp4, _zp6, _zp7, _zp13

WIDTH               = 640
USEFUL_DATABUF_SIZE = 321
DATABUF_SIZE        = $400

raw_ptr1    = _zp2
wordcnt     = _zp2
srcbuf      = _zp2
_x          = _zp2
_y          = _zp3

cur_buf_0l  = _zp4
cur_buf_0h  = _zp6
col         = _zp7
rept        = _zp13

.macro SET_REFILL_RET addr
        ldx     #<addr
        stx     _refill_ret+1
        ldx     #>addr
        stx     _refill_ret+2
.endmacro

.segment "LC"

.proc _init_top
        jsr     _init_huff
        jsr     _init_shiftl3
        jsr     _init_shiftl4
        jsr     _init_div48
        jmp     _init_buf_0
.endproc

.proc _init_shiftl4
        ldy     #$00

:       tya
        bmi     neg
        ldx     #0
        jsr     aslax4
        sta     _shiftl4p_l,y
        txa
        sta     _shiftl4p_h,y
        iny
        bne     :-

neg:    ldx     #$FF
        jsr     aslax4
        sta     _shiftl4n_l-128,y
        txa
        sta     _shiftl4n_h-128,y
        iny
        bne     :-
        rts
.endproc

.proc _init_shiftl3
        ldy     #31
:       tya
        asl
        asl
        asl
        adc     #4
        sta     _shiftl3,y
        dey
        bpl     :-
        rts
.endproc

.proc _init_buf_0
        lda     #<_buf_0
        ldx     #>_buf_0
        jsr     pushax

        lda     #<2048
        jsr     pusha0

        lda     #<USEFUL_DATABUF_SIZE
        ldx     #>USEFUL_DATABUF_SIZE
        jsr     _memset

        lda     #<_buf_0
        ldx     #>(_buf_0+512)
        jsr     pushax

        lda     #>2048
        jsr     pusha0

        lda     #<USEFUL_DATABUF_SIZE
        ldx     #>USEFUL_DATABUF_SIZE
        jmp     _memset
.endproc

.proc _init_div48
        lda     #0
        sta     wordcnt

:       ldx     wordcnt
        lda     #$80
        ldy     #48
        jsr     approx_div16x8_direct
        ldy     wordcnt
        sta     _div48_l,y
        txa
        sta     _div48_h,y
        inc     wordcnt
        bne     :-
        rts
.endproc

.proc _init_huff
        bit     $C083               ; WR-enable LC
        bit     $C083               ; we patch things.
        lda     #0
        sta     val
        sta     val+1
        sta     src_idx
        sta     src_idx+1
        sta     huff_ctrl_bits+1
        sta     huff_ctrl_value+1
        sta     huff_data_bits+1
        lda     #$80
        sta     huff_data_value+1

        ldx     #>_huff_ctrl
        stx     huff_ctrl_bits+2
        inx
        stx     huff_ctrl_value+2

        lda     #>_src
        sta     srcbuf+1
        lda     #<_src
        sta     srcbuf
ctrl_huff_next:
        ldy     src_idx

        lda     (srcbuf),y
        tax                         ; X = numbits

        stx     tmp1                ; 8-numbits
        lda     #8
        sec
        sbc     tmp1
        tay

        lda     val                 ; code = val & 0xFF
        cpy     #0
        beq     ctrl_code_shifted
:       lsr     a                   ; code >>= 8-numbits
        dey
        bne     :-

ctrl_code_shifted:
        sta     code
        txa
        tay
        lda     #$80                ; incr = 256 >> numbits
        dey
        beq     ctrl_incr_shifted
:       lsr     a
        dey
        bne     :-

ctrl_incr_shifted:
        sta     incr
        ldy     src_idx
        iny
        lda     (srcbuf),y          ; A = value

        ldy     code
huff_ctrl_value:
        sta     _huff_ctrl+256,y

        txa                         ; X = numbits
huff_ctrl_bits:
        sta     _huff_ctrl,y

        lda     src_idx             ; src_idx += 2
        clc
        adc     #2
        sta     src_idx
        bcc     :+
        inc     srcbuf+1

:       lda     val                 ; val += incr
        clc
        adc     incr
        sta     val
        bcc     ctrl_huff_next

        lda     huff_ctrl_value+2
        clc
        adc     #2
        cmp     #>(_huff_ctrl+18*256)
        bcs     huff_ctrl_done
        sta     huff_ctrl_value+2
        inc     huff_ctrl_bits+2
        inc     huff_ctrl_bits+2
        jmp     ctrl_huff_next
huff_ctrl_done:

        ; Data huff now
        ldx     #>_huff_data
        stx     huff_data_bits+2
        stx     huff_data_value+2

data_huff_next:
        ldy     src_idx

        lda     (srcbuf),y
        tax                         ; X = numbits

        stx     tmp1                ; 8-numbits
        lda     #8
        sec
        sbc     tmp1
        tay

        lda     val                 ; code = val & 0xFF
        cpy     #0
        beq     data_code_shifted
:       lsr     a                   ; code >>= 8-numbits
        dey
        bne     :-

data_code_shifted:
        sta     code
        txa
        tay
        lda     #$80                ; incr = 256 >> numbits
        dey
        beq     data_incr_shifted
:       lsr     a
        dey
        bne     :-

data_incr_shifted:
        sta     incr
        ldy     src_idx
        iny
        lda     (srcbuf),y          ; A = value

        ldy     code
huff_data_value:
        sta     _huff_data+128,y

        txa                         ; X = numbits
huff_data_bits:
        sta     _huff_data,y

        lda     src_idx             ; src_idx += 2
        clc
        adc     #2
        sta     src_idx
        bcc     :+
        inc     srcbuf+1

:       lda     val                 ; val += incr
        clc
        adc     incr
        sta     val
        bcc     data_huff_next

        lda     huff_data_value+2
        clc
        adc     #1
        cmp     #>(_huff_data+9*256)
        bcs     huff_data_done
        sta     huff_data_value+2
        inc     huff_data_bits+2
        jmp     data_huff_next
huff_data_done:
        rts
.endproc

.proc _copy_data
        bit     $C083
        bit     $C083
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

.segment "CODE"

.proc _consume_extra
        lda     #2
        sta     repeats
repeat_loop:
        lda     #1
        sta     tree
        lda     #<(WIDTH/4)
        sta     col

col_loop2:
        lda     tree
        asl
        adc     #>_huff_ctrl
        sta     _huff_numc
        adc     #1
        sta     _huff_numc_h
        SET_REFILL_RET _getctrlhuff_refilled

        jsr     _getctrlhuff
        sta     tree

        beq     tree_zero_2

tree_not_zero_2:
        dec     col

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
        lda     col
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
        sta     nreps

        cmp     #9
        bcc     nreps_check_done_2
        lda     #8
nreps_check_done_2:
        sta     rep_loop

        ;data tree 1
        ldx     #>(_huff_data+256)
        stx     _huff_numd
        stx     _huff_numd_h
        ; refiller return already set

        ldx     #$00
        stx     rept

do_rep_loop_2:
        dec     col

        txa
        and     #1
        beq     rep_even_2

        jsr     _getdatahuff

rep_even_2:
        ldx     rept
        inx
        cpx     rep_loop
        beq     rep_loop_2_done
        stx     rept
        lda     col
        bne     do_rep_loop_2
        beq     check_c_loop
rep_loop_2_done:
        lda     nreps
        cmp     #9
        beq     tree_zero_2

tree_zero_2_done:
        lda     col
        beq     check_c_loop
        jmp     col_loop2

check_c_loop:
        dec     repeats
        beq     c_loop_done
        jmp     repeat_loop
c_loop_done:
        rts
.endproc

.proc _init_row
        lda     #<(_buf_0+256)      ; start at second page
        ldx     #>(_buf_0+256)
        sta     cur_buf_0l
        stx     cur_buf_0l+1
        lda     #<(_buf_0+256+512)
        ldx     #>(_buf_0+256+512)
        sta     cur_buf_0h
        stx     cur_buf_0h+1

        ldy     _last               ; is last 8bits?
        cpy     #18
        bcs     small_val

        lda     _val_from_last,y    ; no, 16x8 mult
        ldx     _val_hi_from_last,y
        jsr     pushax
        lda     _factor
        sta     _last
        jsr     tosmula0
        jmp     shift_val

small_val:
        lda     _val_from_last,y    ; yes, 8x8 mult
        ldx     _factor
        stx     _last
        jsr     mult8x8r16_direct
shift_val:
        stx     ptr2+1              ; >> 4
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
        cmp     #$FF                ; is multiplier 0xFF?
        bne     check0x100
        cpx     #$00
        bne     check0x100

mult_FF:                            ; Yes, loop avoiding mults
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
        lda     (cur_buf_0l),y      ; tmp32 * val(0xFF) = tmp32<<8-tmp32
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

check0x100:                         ; Is multiplier 0x100?
        cpx     #$01
        bne     slow_mults
        cmp     #$00
        beq     init_done           ; Yes, so nothing to do!

slow_mults:                         ; Arbitrary multiplier
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

.proc _decode_row
        lda     #1
        sta     repeats
r_loop:
        ;  for (r=0; r != 2; r++) {
        ;  factor<<7, aslax7 inlined */
        lda     _factor
        lsr     a
        tax
        lda     #0
        ror

        ;  Update buf1/2[WIDTH/2] = factor<<7 */
        stx     _buf_1+(WIDTH/2)+512
        stx     _buf_2+(WIDTH/2)+512
        sta     _buf_1+(WIDTH/2)
        sta     _buf_2+(WIDTH/2)

        lda     #1
        sta     tree

        lda     #<(WIDTH/2)
        sta     col
        ldx     #>(WIDTH/2)
        stx     colh
        lda     #>(_buf_0+512+256)
        sta     cb0h_off0a+2
        sta     cb0h_off0b+2
        sta     cb0h_off1a+2
        sta     cb0h_off1b+2
        sta     cb0h_off1c+2
        sta     cb0h_off1d+2
        sta     cb0h_off2a+2
        sta     cb0h_off2b+2
        lda     #>(_buf_0+256)
        sta     cb0l_off0a+2
        sta     cb0l_off0b+2
        sta     cb0l_off1a+2
        sta     cb0l_off1b+2
        sta     cb0l_off1c+2
        sta     cb0l_off1d+2
        sta     cb0l_off2a+2
        sta     cb0l_off2b+2

        lda     #>(_buf_1+512+256)
        sta     cb1h_off0a+2
        sta     cb1h_off0b+2
        sta     cb1h_off0c+2
        sta     cb1h_off0d+2
        sta     cb1h_off0e+2
        sta     cb1h_off0f+2
        sta     cb1h_off0g+2
        sta     cb1h_off1a+2
        sta     cb1h_off1b+2
        sta     cb1h_off1c+2
        sta     cb1h_off1d+2
        sta     cb1h_off1e+2
        sta     cb1h_off1f+2
        sta     cb1h_off1g+2
        sta     cb1h_off1h+2
        sta     cb1h_off1i+2
        sta     cb1h_off1j+2
        sta     cb1h_off1k+2
        sta     cb1h_off2a+2
        sta     cb1h_off2b+2
        sta     cb1h_off2c+2
        sta     cb1h_off2d+2
        lda     #>(_buf_1+256)
        sta     cb1l_off0a+2
        sta     cb1l_off0b+2
        sta     cb1l_off0c+2
        sta     cb1l_off0d+2
        sta     cb1l_off0e+2
        sta     cb1l_off0f+2
        sta     cb1l_off0g+2
        sta     cb1l_off1a+2
        sta     cb1l_off1b+2
        sta     cb1l_off1c+2
        sta     cb1l_off1d+2
        sta     cb1l_off1e+2
        sta     cb1l_off1f+2
        sta     cb1l_off1h+2
        sta     cb1l_off1i+2
        sta     cb1l_off1j+2
        sta     cb1l_off1k+2
        sta     cb1l_off2a+2
        sta     cb1l_off2b+2
        sta     cb1l_off2c+2
        sta     cb1l_off2d+2

        lda     #>(_buf_2+512+256)
        sta     cb2h_off0a+2
        sta     cb2h_off0b+2
        sta     cb2h_off0c+2
        sta     cb2h_off0d+2
        sta     cb2h_off0e+2
        sta     cb2h_off1a+2
        sta     cb2h_off1b+2
        sta     cb2h_off1c+2
        sta     cb2h_off1d+2
        sta     cb2h_off1e+2
        sta     cb2h_off1f+2
        sta     cb2h_off1g+2
        sta     cb2h_off2b+2
        sta     cb2h_off2c+2
        lda     #>(_buf_2+256)
        sta     cb2l_off0a+2
        sta     cb2l_off0b+2
        sta     cb2l_off0c+2
        sta     cb2l_off0d+2
        sta     cb2l_off0e+2
        sta     cb2l_off1a+2
        sta     cb2l_off1b+2
        sta     cb2l_off1c+2
        sta     cb2l_off1d+2
        sta     cb2l_off1f+2
        sta     cb2l_off1g+2
        sta     cb2l_off2b+2
        sta     cb2l_off2c+2

col_loop1:
        lda     tree
        asl
        adc     #>_huff_ctrl
        sta     _huff_numc
        adc     #1
        sta     _huff_numc_h
        ldx     #<_getctrlhuff_refilled
        stx     _refill_ret+1
        ldx     #>_getctrlhuff_refilled
        stx     _refill_ret+2

        jsr     _getctrlhuff
        sta     tree

        bne     tree_not_zero
        jmp     tree_zero
tree_not_zero:
        sec
        lda     col
        bne     declow
        jsr     dec_buf_pages
declow:
        sbc     #2
        sta     col

        lda     tree
        cmp     #8
        bne     tree_not_eight

        ldx     #<_getdatahuff8_refilled
        stx     _refill_ret+1
        ldx     #>_getdatahuff8_refilled
        stx     _refill_ret+2

        ;  tree == 8
        jsr     _getdatahuff8
        ldx     _factor
        jsr     mult8x8r16_direct
        ldy     col
cb1l_off1a: 
        sta     $FF01,y
        txa
cb1h_off1a: 
        sta     $FF01,y

        jsr     _getdatahuff8
        ldx     _factor
        jsr     mult8x8r16_direct
        ldy     col
cb1l_off0a: 
        sta     $FF00,y
        txa
cb1h_off0a:
        sta     $FF00,y

        jsr     _getdatahuff8
        ldx     _factor
        jsr     mult8x8r16_direct
        ldy     col
cb2l_off1a:
        sta     $FF01,y
        txa
cb2h_off1a:
        sta     $FF01,y

        jsr     _getdatahuff8
        ldx     _factor
        jsr     mult8x8r16_direct
        ldy     col
cb2l_off0a:
        sta     $FF00,y
        txa
cb2h_off0a:
        sta     $FF00,y

        jmp     tree_done

tree_not_eight:
        ; huff_num = tree+1
        adc     #>(_huff_data+256)
        sta     _huff_numd
        sta     _huff_numd_h

        ldx     #<_getdatahuff_refilled
        stx     _refill_ret+1
        ldx     #>_getdatahuff_refilled
        stx     _refill_ret+2

        ;  Get the four tk vals in advance
        ; a
        jsr     _getdatahuff
        tax
        bpl     pos1
        lda     _shiftl4n_l-128,x
        sta     tk1_l+1
        lda     _shiftl4n_h-128,x
        jmp     finish_bh1
pos1:
        lda     _shiftl4p_l,x
        sta     tk1_l+1
        lda     _shiftl4p_h,x

finish_bh1:
        sta     tk1_h+1

        jsr     _getdatahuff
        tax
        bpl     pos2
        lda     _shiftl4n_l-128,x
        sta     tk2_l+1
        lda     _shiftl4n_h-128,x
        jmp     finish_bh2
pos2:
        lda     _shiftl4p_l,x
        sta     tk2_l+1
        lda     _shiftl4p_h,x

finish_bh2:
        sta     tk2_h+1

        jsr     _getdatahuff
        tax
        bpl     pos3
        lda     _shiftl4n_l-128,x
        sta     tk3_l+1
        lda     _shiftl4n_h-128,x
        jmp     finish_bh3
pos3:
        lda     _shiftl4p_l,x
        sta     tk3_l+1
        lda     _shiftl4p_h,x

finish_bh3:
        sta     tk3_h+1

        jsr     _getdatahuff
        tax
        bpl     pos4
        lda     _shiftl4n_l-128,x
        sta     tk4_l+1
        lda     _shiftl4n_h-128,x
        jmp     finish_bh4
pos4:
        lda     _shiftl4p_l,x
        sta     tk4_l+1
        lda     _shiftl4p_h,x

finish_bh4:
        sta     tk4_h+1

        clc
        ldy     col
cb0l_off2a:
        lda     $FF02,y
cb1l_off2a:
        adc     $FF02,y
        tax
cb0h_off2a:
        lda     $FF02,y
cb1h_off2a:
        adc     $FF02,y
        cmp     #$80
        ror     a
        sta     tmp1
        txa
        ror     a

        clc
cb0l_off1a:
        adc     $FF01,y
        tax
        lda     tmp1
cb0h_off1a:
        adc     $FF01,y
        cmp     #$80
        ror     a
        sta     tmp1
        txa
        ror     a

        clc
tk1_l:  adc     #$FF
cb1l_off1b:
        sta     $FF01,y
        lda     tmp1
tk1_h:  adc     #$FF
cb1h_off1b:
        sta     $FF01,y

        ;  Second with col - 1*/
        clc
cb0l_off1b:
        lda     $FF01,y
cb1l_off1c:
        adc     $FF01,y
        tax
cb0h_off1b:
        lda     $FF01,y
cb1h_off1c:
        adc     $FF01,y
        cmp     #$80
        ror     a
        sta     tmp1
        txa
        ror     a

        clc
cb0l_off0a:
        adc     $FF00,y
        tax
        lda     tmp1
cb0h_off0a:
        adc     $FF00,y
        cmp     #$80
        ror     a
        sta     tmp1
        txa
        ror     a

        ;  Store to cur_buf_x */
        clc
tk2_l:  adc     #$FF
cb1l_off0b:
        sta     $FF00,y

        lda     tmp1
tk2_h:  adc     #$FF
cb1h_off0b:
        sta     $FF00,y

        ; b
        clc
cb1l_off2b:
        lda     $FF02,y
cb2l_off2b:
        adc     $FF02,y
        tax
cb1h_off2b:
        lda     $FF02,y
cb2h_off2b:
        adc     $FF02,y
        cmp     #$80
        ror     a
        sta     tmp1
        txa
        ror     a

        clc
cb1l_off1d:
        adc     $FF01,y
        tax
        lda     tmp1
cb1h_off1d:
        adc     $FF01,y
        cmp     #$80
        ror     a
        sta     tmp1
        txa
        ror     a

        clc
tk3_l:  adc     #$FF
cb2l_off1b:
        sta     $FF01,y

        lda     tmp1
tk3_h:  adc     #$FF
cb2h_off1b:
        sta     $FF01,y

        ;  Second with col - 1*/
        clc
cb1l_off1e:
        lda     $FF01,y
cb2l_off1c:
        adc     $FF01,y
        tax
cb1h_off1e:
        lda     $FF01,y
cb2h_off1c:
        adc     $FF01,y
        cmp     #$80
        ror     a
        sta     tmp1
        txa
        ror     a

        clc
cb1l_off0c:
        adc     $FF00,y
        tax
        lda     tmp1
cb1h_off0c:
        adc     $FF00,y
        cmp     #$80
        ror     a
        sta     tmp1
        txa
        ror     a

        ;  Store to cur_buf_x */
        clc
tk4_l:  adc     #$FF
cb2l_off0b:
        sta     $FF00,y

        lda     tmp1
tk4_h:  adc     #$FF
cb2h_off0b:
        sta     $FF00,y

        jmp     tree_done

tree_zero:
nine_reps_loop:
        ldx     colh
        bne     col_gt1a
        lda     col
        cmp     #3
        bcs     col_gt1a
        lda     #1 ;  nreps */
        jmp     check_nreps
col_gt1a:
        ;  data tree 0
        ldx     #>(_huff_data)
        stx     _huff_numd
        stx     _huff_numd_h
        ldx     #<_getdatahuff_refilled
        stx     _refill_ret+1
        ldx     #>_getdatahuff_refilled
        stx     _refill_ret+2

        jsr     _getdatahuff
        clc
        adc     #1
check_nreps:
        sta     nreps

        cmp     #9
        bcc     nreps_check_done
        lda     #8
nreps_check_done:
        sta     rep_loop

        ldy     #$00
        sty     rept

        ;  data tree 1
        ldx     #>(_huff_data+256)
        stx     _huff_numd
        stx     _huff_numd_h

        lda     col
do_rep_loop:
        sec
        bne     declow2
        jsr     dec_buf_pages
declow2:
        sbc     #2
        sta     col
        tay

        clc
cb0l_off2b:
        lda     $FF02,y
cb1l_off2c:
        adc     $FF02,y
        tax
cb0h_off2b:
        lda     $FF02,y
cb1h_off2c:
        adc     $FF02,y
        cmp     #$80
        ror     a
        sta     tmp1
        txa
        ror     a

        clc
cb0l_off1c:
        adc     $FF01,y
        tax
        lda     tmp1
cb0h_off1c:
        adc     $FF01,y
        cmp     #$80
        ror     a
cb1h_off1f:
        sta     $FF01,y
        txa
        ror     a
cb1l_off1f:
        sta     $FF01,y

        ;  Second */
        clc
        ;  cb1l_off1g:   lda     $FF01,y already good
cb0l_off1d:
        adc     $FF01,y
        tax
cb1h_off1g:
        lda     $FF01,y
cb0h_off1d:
        adc     $FF01,y
        cmp     #$80
        ror     a
        sta     tmp1
        txa
        ror     a

        clc
cb0l_off0b:
        adc     $FF00,y
        tax
        lda     tmp1
cb0h_off0b:
        adc     $FF00,y
        cmp     #$80
        ror     a
cb1h_off0d:
        sta     $FF00,y
        txa
        ror     a
cb1l_off0d:
        sta     $FF00,y

        ; d
        clc
cb1l_off2d:
        lda     $FF02,y
cb2l_off2c:
        adc     $FF02,y
        tax
cb1h_off2d:
        lda     $FF02,y
cb2h_off2c:
        adc     $FF02,y
        cmp     #$80
        ror     a
        sta     tmp1
        txa
        ror     a

        clc
cb1l_off1h:
        adc     $FF01,y
        tax
        lda     tmp1
cb1h_off1h:
        adc     $FF01,y
        cmp     #$80
        ror     a
cb2h_off1d:
        sta     $FF01,y
        txa
        ror     a
cb2l_off1d:
        sta     $FF01,y

        ;  Second */
        clc
        ;  cb2l_off1e:   lda     $FF01,y
cb1l_off1i:
        adc     $FF01,y
        tax
cb2h_off1e:
        lda     $FF01,y
cb1h_off1i:
        adc     $FF01,y
        cmp     #$80
        ror     a
        sta     tmp1
        txa
        ror     a

        clc
cb1l_off0e:
        adc     $FF00,y
        tax
        lda     tmp1
cb1h_off0e:
        adc     $FF00,y
        cmp     #$80
        ror     a
cb2h_off0c:
        sta     $FF00,y
        txa
        ror     a
cb2l_off0c:
        sta     $FF00,y

        lda     rept
        and     #1
        beq     rep_even

        ;  tk = getbithuff(8) << 4;
        jsr     _getdatahuff
        tax
        bpl     pos5
        lda     _shiftl4n_h-128,x
        sta     tmp2
        lda     _shiftl4n_l-128,x
        jmp     finish_bh5
pos5:
        lda     _shiftl4p_h,x
        sta     tmp2
        lda     _shiftl4p_l,x
finish_bh5:
        tax

        ; e
        clc
cb1l_off0f:
        adc     $FF00,y
cb1l_off0g:
        sta     $FF00,y
        lda     tmp2
cb1h_off0f:
        adc     $FF00,y
cb1h_off0g:
        sta     $FF00,y

        txa
cb1l_off1j:
        adc     $FF01,y
cb1l_off1k:
        sta     $FF01,y
        lda     tmp2
cb1h_off1j:
        adc     $FF01,y
cb1h_off1k:
        sta     $FF01,y

        txa
cb2l_off0d:
        adc     $FF00,y
cb2l_off0e:
        sta     $FF00,y
        lda     tmp2
cb2h_off0d:
        adc     $FF00,y
cb2h_off0e:
        sta     $FF00,y

        txa
cb2l_off1f:
        adc     $FF01,y
cb2l_off1g:
        sta     $FF01,y
        lda     tmp2
cb2h_off1f:
        adc     $FF01,y
cb2h_off1g:
        sta     $FF01,y

rep_even:
        ldx     rept
        inx
        cpx     rep_loop
        beq     rep_loop_done
        stx     rept
        lda     col
        beq     check_high
        jmp     do_rep_loop
check_high:
        ldx     colh
        beq     col_loop1_done
        tay     ;  fix ZERO flag needed at do_rep_loop
        jmp     do_rep_loop
rep_loop_done:
        lda     nreps
        cmp     #9
        bne     nine_reps_loop_done
        jmp     nine_reps_loop
nine_reps_loop_done:
tree_done:
        lda     col
        beq     check_high2
        jmp     col_loop1
check_high2:
        ldx     colh
        beq     col_loop1_done
        jmp     col_loop1
col_loop1_done:

        clc

        ldx     repeats
        ;  logic inverted from C because here we dec R */
        beq     store_plus_2

        lda     _row_idx
        sta     store_val+1
        lda     _row_idx+1
        sta     store_val+2
        jmp     store_set
store_plus_2:
        lda     _row_idx_plus2
        sta     store_val+1
        lda     _row_idx_plus2+1
        sta     store_val+2

store_set:
        lda     #2
        sta     _y

        ldy     #<(_buf_1)
        sty     cur_buf_0l
        sty     cur_buf_0h
        lda     #>(_buf_1)
        sta     cur_buf_0l+1
        clc
        adc     #>(DATABUF_SIZE/2)
        sta     cur_buf_0h+1

loop5:
        lda     #4
        sta     _x
x_loop_outer:
        ldy     #0
x_loop:
        sty     tmp1
        tya
        lsr     a
        tay
        lda     (cur_buf_0h),y
        ldy     _factor
        cpy     #48
        bne     slowdiv
        tay
        ldx     _div48_h,y
        lda     _div48_l,y
        jmp     check_clamp
slowdiv:
        tax
        lda     (cur_buf_0l),y
        jsr     approx_div16x8_direct
check_clamp:
        ldy     tmp1
        cpx     #0
        beq     store_val
        lda     #$FF
        cpx     #$80
        adc     #$0

store_val:
        sta     $FFFF,y

        iny
        iny
        cpy     #<(WIDTH/4)
        bne     x_loop

        clc
        lda     cur_buf_0l
        adc     #<(WIDTH/8)
        sta     cur_buf_0l
        sta     cur_buf_0h
        lda     cur_buf_0l+1
        adc     #>(WIDTH/8)
        sta     cur_buf_0l+1
        adc     #>(DATABUF_SIZE/2)
        sta     cur_buf_0h+1

        clc
        lda     store_val+1
        adc     #<(WIDTH/4)
        sta     store_val+1
        lda     store_val+2
        adc     #>(WIDTH/4)
        sta     store_val+2

        dec     _x
        bne     x_loop_outer

        dec     _y
        beq     loop5_done

        inc     store_val+1
        bne     :+
        inc     store_val+2

:       ldy     #<(_buf_2)
        sty     cur_buf_0l
        sty     cur_buf_0h
        lda     #>(_buf_2)
        sta     cur_buf_0l+1
        clc
        adc     #>(DATABUF_SIZE/2)
        sta     cur_buf_0h+1

        jmp     loop5

loop5_done:
        clc
        ldx     #>(_buf_0+1)    ;  cur_buf[0]+1 */
        lda     #<(_buf_0+1)
        jsr     pushax

        lda     #<(_buf_2) ;  curbuf_2 */
        ldx     #>(_buf_2)
        jsr     pushax

        lda     #<(USEFUL_DATABUF_SIZE-1)
        ldx     #>(USEFUL_DATABUF_SIZE-1)
        jsr     _memcpy

        ldx     #>(_buf_0+512+1) ;  cur_buf[0]+1 */
        lda     #<(_buf_0+512+1)
        jsr     pushax

        lda     #<(_buf_2+512) ;  curbuf_2 */
        ldx     #>(_buf_2+512)
        jsr     pushax

        lda     #<(USEFUL_DATABUF_SIZE-1)
        ldx     #>(USEFUL_DATABUF_SIZE-1)
        jsr     _memcpy
        ;  }
        dec     repeats
        bmi     r_loop_done
        jmp     r_loop
r_loop_done:
        rts
.endproc

.proc dec_buf_pages
        dec     colh
        dec     _decode_row::cb0h_off0a+2
        dec     _decode_row::cb0h_off0b+2
        dec     _decode_row::cb0h_off1a+2
        dec     _decode_row::cb0h_off1b+2
        dec     _decode_row::cb0h_off1c+2
        dec     _decode_row::cb0h_off1d+2
        dec     _decode_row::cb0h_off2a+2
        dec     _decode_row::cb0h_off2b+2
        dec     _decode_row::cb0l_off0a+2
        dec     _decode_row::cb0l_off0b+2
        dec     _decode_row::cb0l_off1a+2
        dec     _decode_row::cb0l_off1b+2
        dec     _decode_row::cb0l_off1c+2
        dec     _decode_row::cb0l_off1d+2
        dec     _decode_row::cb0l_off2a+2
        dec     _decode_row::cb0l_off2b+2
        dec     _decode_row::cb1h_off0a+2
        dec     _decode_row::cb1h_off0b+2
        dec     _decode_row::cb1h_off0c+2
        dec     _decode_row::cb1h_off0d+2
        dec     _decode_row::cb1h_off0e+2
        dec     _decode_row::cb1h_off0f+2
        dec     _decode_row::cb1h_off0g+2
        dec     _decode_row::cb1h_off1a+2
        dec     _decode_row::cb1h_off1b+2
        dec     _decode_row::cb1h_off1c+2
        dec     _decode_row::cb1h_off1d+2
        dec     _decode_row::cb1h_off1e+2
        dec     _decode_row::cb1h_off1f+2
        dec     _decode_row::cb1h_off1g+2
        dec     _decode_row::cb1h_off1h+2
        dec     _decode_row::cb1h_off1i+2
        dec     _decode_row::cb1h_off1j+2
        dec     _decode_row::cb1h_off1k+2
        dec     _decode_row::cb1h_off2a+2
        dec     _decode_row::cb1h_off2b+2
        dec     _decode_row::cb1h_off2c+2
        dec     _decode_row::cb1h_off2d+2
        dec     _decode_row::cb1l_off0a+2
        dec     _decode_row::cb1l_off0b+2
        dec     _decode_row::cb1l_off0c+2
        dec     _decode_row::cb1l_off0d+2
        dec     _decode_row::cb1l_off0e+2
        dec     _decode_row::cb1l_off0f+2
        dec     _decode_row::cb1l_off0g+2
        dec     _decode_row::cb1l_off1a+2
        dec     _decode_row::cb1l_off1b+2
        dec     _decode_row::cb1l_off1c+2
        dec     _decode_row::cb1l_off1d+2
        dec     _decode_row::cb1l_off1e+2
        dec     _decode_row::cb1l_off1f+2
        dec     _decode_row::cb1l_off1h+2
        dec     _decode_row::cb1l_off1i+2
        dec     _decode_row::cb1l_off1j+2
        dec     _decode_row::cb1l_off1k+2
        dec     _decode_row::cb1l_off2a+2
        dec     _decode_row::cb1l_off2b+2
        dec     _decode_row::cb1l_off2c+2
        dec     _decode_row::cb1l_off2d+2
        dec     _decode_row::cb2h_off0a+2
        dec     _decode_row::cb2h_off0b+2
        dec     _decode_row::cb2h_off0c+2
        dec     _decode_row::cb2h_off1a+2
        dec     _decode_row::cb2h_off1b+2
        dec     _decode_row::cb2h_off1c+2
        dec     _decode_row::cb2h_off1d+2
        dec     _decode_row::cb2h_off1e+2
        dec     _decode_row::cb2h_off2b+2
        dec     _decode_row::cb2h_off2c+2
        dec     _decode_row::cb2l_off0a+2
        dec     _decode_row::cb2l_off0b+2
        dec     _decode_row::cb2l_off0c+2
        dec     _decode_row::cb2l_off1a+2
        dec     _decode_row::cb2l_off1b+2
        dec     _decode_row::cb2l_off1c+2
        dec     _decode_row::cb2l_off1d+2
        dec     _decode_row::cb2l_off2b+2
        dec     _decode_row::cb2l_off2c+2
        dec     _decode_row::cb2l_off0d+2
        dec     _decode_row::cb2l_off0e+2
        dec     _decode_row::cb2h_off0d+2
        dec     _decode_row::cb2h_off0e+2
        dec     _decode_row::cb2l_off1f+2
        dec     _decode_row::cb2l_off1g+2
        dec     _decode_row::cb2h_off1f+2
        dec     _decode_row::cb2h_off1g+2
        rts
.endproc

.bss

tree:           .res 1
rep_loop:       .res 1
nreps:          .res 1
colh:           .res 1
repeats:        .res 1
val:            .res 2
src_idx:        .res 2
incr:           .res 1
code:           .res 1
