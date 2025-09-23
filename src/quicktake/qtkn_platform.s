        .export       _copy_data
        .export       _consume_extra
        .export       _init_row
        .export       _decode_row

        .export       _init_top
        .export       _init_shiftl4
        .export       _init_buf_0
        .export       _init_div48
        .export       _init_huff

        .import       _row_idx, _row_idx_plus2

        .import       _getdatahuff, _getdatahuff8
        .import       _getctrlhuff

        .import       _discarddatahuff, _discarddatahuff8
        .import       _huff_numc, _huff_numc_h
        .import       _huff_numd, _huff_numd_h
        .import       _huff_numdd
        .import       _huff_data, _huff_ctrl, _src
        .import       _factor, _last
        .import       _val_from_last, _val_hi_from_last
        .import       _buf_0, _buf_1
        .import       _div48_l, _div48_h
        .import       _dyndiv_l, _dyndiv_h
        .import       _shiftl4_l, _shiftl4_h

        .import       mult16x16mid16_direct, mult8x8r16_direct
        .import       tosmula0, pushax, pusha0
        .import       _memset, _memcpy, aslax4
.ifdef APPROX_DIVISION
        .import       approx_div16x8_direct
.else
        .import       tosdiva0
.endif

        .importzp     tmp1, sreg, ptr2, tmp2, tmp3, tmp4
        .importzp     _zp2, _zp3, _zp4, _zp6, _zp7, _zp13

WIDTH               = 320
USEFUL_DATABUF_SIZE = 321
DATABUF_SIZE        = $400

raw_ptr1    = _zp2
wordcnt     = _zp2
srcbuf      = _zp2
_x          = _zp2

cur_buf_0l  = _zp4
cur_buf_0h  = _zp6
col         = _zp7
rept        = _zp13

.segment "LC"

.proc _init_top
        jsr     _init_huff
        jsr     _init_shiftl4
        jsr     _init_div48
        jmp     _init_buf_0
.endproc

.proc _init_shiftl4
        ldy     #$00

:       ldx     #0
        tya
        bpl     :+
        dex
:       jsr     aslax4
        sta     _shiftl4_l,y
        txa
        sta     _shiftl4_h,y
        iny
        bne     :--
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
.ifdef APPROX_DIVISION
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
        bne     overflows
        inc     wordcnt
        bne     :-
        rts

overflows:                        ; Fill the rest
        sta     _dyndiv_h,y
        iny
        bne     overflows
.endif
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

.proc _consume_extra
        lda     #2
        sta     repeats
repeat_loop:
        lda     #1
        sta     tree
        lda     #<(WIDTH/2)
        sta     col

col_loop2:
        lda     tree
        asl
        adc     #>_huff_ctrl
        sta     _huff_numc
        adc     #1
        sta     _huff_numc_h

        jsr     _getctrlhuff
        sta     tree

        beq     tree_zero_2

tree_not_zero_2:
        dec     col

        ;huff_ptr = huff[tree + 10]
        cmp     #8
        bne     norm_huff

        jsr     _discarddatahuff8
        jsr     _discarddatahuff8
        jsr     _discarddatahuff8
        jsr     _discarddatahuff8
        jmp     tree_zero_2_done

norm_huff:
        adc     #>(_huff_data+256)
        sta     _huff_numdd

        jsr     _discarddatahuff
        jsr     _discarddatahuff
        jsr     _discarddatahuff
        jsr     _discarddatahuff

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

        jsr     _getdatahuff
        inx

check_nreps_2:
        stx     nreps

        cpx     #9
        bcc     nreps_check_done_2
        ldx     #8
nreps_check_done_2:
        stx     rep_loop

        ;data tree 1
        ldx     #>(_huff_data+256)
        stx     _huff_numdd
        ; refiller return already set

        ldx     #$00
        stx     rept

do_rep_loop_2:
        dec     col

        txa
        and     #1
        beq     rep_even_2

        jsr     _discarddatahuff

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

.segment "CODE"

.proc _copy_data
        lda     _row_idx
        ldx     _row_idx+1
        ldy     _factor
        sty     div_factor+1
        ldy     repeats
        ;  logic inverted from C because here we dec R */
        bne     store_set
store_plus_2:
        lda     _row_idx_plus2
        ldx     _row_idx_plus2+1

store_set:
        sta     store_val+1
        inx                       ; Start at page 2
        stx     store_val+2

.ifndef APPROX_DIVISION
        lda     #>(_buf_1+256)    ; Start at page 2
        sta     cb0l_c+2
.endif
        lda     #>(_buf_1+256+512)
        sta     cb0h_c+2

        ldy     #<(WIDTH)
        sty     _x
        lda     #>(WIDTH)
        sta     _x+1

x_loop:
        dey
        sty     _x
.ifdef APPROX_DIVISION
cb0h_c: ldx     $FF00,y
div_factor:
        ldy     #$FF
divtable_l:
        lda     _div48_l,x    ; Preload even if we'll clamp, which is rare
divtable_h:
        ldy     _div48_h,x
        beq     reload_x_and_store
clamp:
        lda     #$FF

.else
cb0h_c: ldx     $FF00,y
cb0l_c: lda     $FF00,y
        jsr     pushax
div_factor:
        lda     #$FF
        jsr     tosdiva0
check_clamp:
        cpx     #0
        beq     reload_x_and_store
clamp:
        lda     #$FF
        cpx     #$80          ; X < 0 ==> 0, X > 0 ==> 255
        adc     #$0
.endif

reload_x_and_store:
        ldy     _x
store_val:
        sta     $FFFF,y

        ; cpy     #0
        bne     x_loop
.ifndef APPROX_DIVISION
        dec     cb0l_c+2
.endif
        dec     cb0h_c+2
        dec     store_val+2
        dec     _x+1
        bpl     x_loop

stores_done:
        rts
.endproc

.proc _init_dyndiv
.ifdef APPROX_DIVISION
        sta     abck
        stx     xbck
        sty     ybck
        sta     fact+1

        lda     #0
        sta     wordcnt

:       ldx     wordcnt
        lda     #$80
fact:   ldy     #$FF
        jsr     approx_div16x8_direct
        ldy     wordcnt
        sta     _dyndiv_l,y
        txa
        sta     _dyndiv_h,y
        bne     overflows         ; Stop if result > 256
        inc     wordcnt
        bne     :-
        jmp     done

overflows:                        ; Fill the rest, overflowed
        sta     _dyndiv_h,y
        iny
        bne     overflows

done:
abck = *+1
        lda     #$FF
xbck = *+1
        ldx     #$FF
ybck = *+1
        ldy     #$FF
.endif
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

.ifdef APPROX_DIVISION
        ldx     #>_div48_h
        stx     _copy_data::divtable_h+2
        ldx     #>_div48_l
        stx     _copy_data::divtable_l+2
.endif
        ldy     _last               ; is last 8bits?
        cpy     #18
        bcs     small_val

        lda     _val_from_last,y    ; no, 16x8 mult
        ldx     _val_hi_from_last,y
        jsr     pushax
        lda     _factor             ; 
        sta     _last
.ifdef APPROX_DIVISION
        cmp     #48
        beq     :+
        ldx     #>_dyndiv_h         ; Set table to dyn
        stx     _copy_data::divtable_h+2
        ldx     #>_dyndiv_l
        stx     _copy_data::divtable_l+2
        cmp     last_dyndiv
        beq     :+
        sta     last_dyndiv
        jsr     _init_dyndiv        ; Init current factor division table
:
 .endif
        jsr     tosmula0
        jmp     check_multiplier

small_val:                          ; Last is 8bit, do a small mult
        ldx     _factor
        stx     _last
        cpx     _last
.ifdef APPROX_DIVISION
        cpx     #48
        beq     :+
        lda     #>_dyndiv_h
        sta     _copy_data::divtable_h+2
        lda     #>_dyndiv_l
        sta     _copy_data::divtable_l+2
        cpx     last_dyndiv
        beq     :+
        txa
        sta     last_dyndiv
        jsr     _init_dyndiv
:
.endif
        lda     _val_from_last,y    ; yes, 8x8 mult
        jsr     mult8x8r16_direct

check_multiplier:
        cpx     #$0F                ; is multiplier 0xFF?
        bne     check_0x100         ; Check before shifting
        cmp     #$F0
        bcc     check_0x100

mult_0xFF:
        ldy     #<(USEFUL_DATABUF_SIZE-1)
        lda     #>(USEFUL_DATABUF_SIZE-1)
        sta     wordcnt+1

setup_curbuf_x_ff:
        ; load
        dey
        sty     wordcnt
        lda     (cur_buf_0h),y
        tax
        lda     (cur_buf_0l),y
        ; tmp32 in AX
        stx     tmp1                ; Store to subtract
        sec
        sbc     tmp1                ; subtract high word from low : -(tmp>>8)
        bcs     :+
        dex

:       sta     (cur_buf_0l),y
        txa
        sta     (cur_buf_0h),y

        cpy     #0
        bne     setup_curbuf_x_ff
        dec     cur_buf_0l+1
        dec     cur_buf_0h+1
        dec     wordcnt+1
        bpl     setup_curbuf_x_ff
        rts

check_0x100:
        cpx     #10                 ; or 0x100?
        bne     :+
        cmp     #10
        bcc     init_done

:       stx     ptr2+1              ; Arbitrary multiplier, >> 4
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

slow_mults:                         ; and multiply
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
        lda     (cur_buf_0l),y
        ; multiply
        jsr     mult16x16mid16_direct
        ldy     wordcnt

store_buf:
        sta     (cur_buf_0l),y
        txa
        sta     (cur_buf_0h),y

        cpy     #0
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
        sta     mult_factor1+1
        sta     mult_factor2+1
        sta     mult_factor3+1
        sta     mult_factor4+1
        lsr     a
        tax
        lda     #0
        ror

        ; buf_1[WIDTH] = factor<<7
        ; buf_0[WIDTH+1] = factor<<7
        stx     _buf_1+(WIDTH)+512
        stx     _buf_0+(WIDTH+1)+512
        sta     _buf_1+(WIDTH)
        sta     _buf_0+(WIDTH+1)

        ; tree = 1,
        lda     #1
        sta     tree

        ; col = WIDTH
        lda     #<(WIDTH)
        sta     col
        ldx     #>(WIDTH)
        stx     colh

        ; Init the numerous patched locations
        ; Worth the ugliness as there are 86
        ; locations, and we run that path
        ; 320*120*2 times so doing $nnnn,y
        ; instead of ($nn), y we spare:
        ; (320*120*2)*(2*86): 13 SECONDS
        ; (and spend only 0.165s shifting
        ; the high bytes, only 120*2 times)
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

        ; the 'cb2' labels apply to buf_0[]
        ; being updated
        lda     #>(_buf_0+512+256)
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
        lda     #>(_buf_0+256)
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
        ; 320 loop, repeated twice, function
        ; called 120 times: every line here
        ; costs 153-400ms depending on the
        ; number of cycles.

        lda     tree
        asl
        adc     #>_huff_ctrl
        sta     _huff_numc
        adc     #1
        sta     _huff_numc_h

        jsr     _getctrlhuff
        sta     tree

        bne     tree_not_zero
        jmp     tree_zero

dechigh:
        jsr     dec_buf_pages
        jmp     declow

tree_not_zero:
        sec
        lda     col
        beq     dechigh
declow:
        sbc     #2
        sta     col
        tay
        lda     tree
        cmp     #8
        bne     tree_not_eight

        ;  tree == 8
        jsr     _getdatahuff8
mult_factor1:
        ldx     #$FF
        jsr     mult8x8r16_direct
        ldy     col
cb1l_off1a: 
        sta     $FF01,y
        txa
cb1h_off1a: 
        sta     $FF01,y

        jsr     _getdatahuff8
mult_factor2:
        ldx     #$FF
        jsr     mult8x8r16_direct
        ldy     col
cb1l_off0a: 
        sta     $FF00,y
        txa
cb1h_off0a:
        sta     $FF00,y

        jsr     _getdatahuff8
mult_factor3:
        ldx     #$FF
        jsr     mult8x8r16_direct
        ldy     col
cb2l_off1a:
        sta     $FF02,y
        txa
cb2h_off1a:
        sta     $FF02,y

        jsr     _getdatahuff8
mult_factor4:
        ldx     #$FF
        jsr     mult8x8r16_direct
        ldy     col
cb2l_off0a:
        sta     $FF01,y
        txa
cb2h_off0a:
        sta     $FF01,y

        lda     col               ; is col loop done?
        beq     :+
        jmp     col_loop1
:       ldx     colh
        beq     :+
        jmp     col_loop1
:       jmp     finish_col_loop

tree_not_eight:
        ; huff_num = tree+1
        adc     #>(_huff_data+256)
        sta     _huff_numd
        sta     _huff_numd_h

        clc
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
        sta     tk1_h+1
        txa
        ror     a
        sta     tk1_l+1
        
        jsr     _getdatahuff
        ldy     col
        lda     _shiftl4_l,x

        clc
tk1_l:  adc     #$FF
cb1l_off1b:
        sta     $FF01,y

        lda     _shiftl4_h,x
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
        sta     tk2_h+1
        txa
        ror     a
        sta     tk2_l+1
        
        jsr     _getdatahuff
        ldy     col
        lda     _shiftl4_l,x

        clc
tk2_l:  adc     #$FF
cb1l_off0b:
        sta     $FF00,y

        lda     _shiftl4_h,x
tk2_h:  adc     #$FF
cb1h_off0b:
        sta     $FF00,y

        ; b
        clc
cb1l_off2b:
        lda     $FF02,y
cb2l_off2b:
        adc     $FF03,y
        tax
cb1h_off2b:
        lda     $FF02,y
cb2h_off2b:
        adc     $FF03,y
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
        sta     tk3_h+1
        txa
        ror     a
        sta     tk3_l+1
        
        jsr     _getdatahuff
        ldy     col
        lda     _shiftl4_l,x

        clc
tk3_l:  adc     #$FF
cb2l_off1b:
        sta     $FF02,y

        lda     _shiftl4_h,x
tk3_h:  adc     #$FF
cb2h_off1b:
        sta     $FF02,y

        ;  Second with col - 1*/
        clc
cb1l_off1e:
        lda     $FF01,y
cb2l_off1c:
        adc     $FF02,y
        tax
cb1h_off1e:
        lda     $FF01,y
cb2h_off1c:
        adc     $FF02,y
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
        sta     tk4_h+1
        txa
        ror     a
        sta     tk4_l+1
        
        jsr     _getdatahuff
        ldy     col
        lda     _shiftl4_l,x

        clc
tk4_l:  adc     #$FF
cb2l_off0b:
        sta     $FF01,y

        lda     _shiftl4_h,x
tk4_h:  adc     #$FF
cb2h_off0b:
        sta     $FF01,y

        lda     col               ; is col loop done?
        beq     :+
        jmp     col_loop1
:       ldx     colh
        beq     :+
        jmp     col_loop1
:       jmp     finish_col_loop

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

        jsr     _getdatahuff
        inx
check_nreps:
        stx     nreps

        cpx     #9
        bcc     nreps_check_done
        ldx     #8
nreps_check_done:
        stx     rep_loop

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

        ;c
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
        adc     $FF03,y
        tax
cb1h_off2d:
        lda     $FF02,y
cb2h_off2c:
        adc     $FF03,y
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
        sta     $FF02,y
        txa
        ror     a
cb2l_off1d:
        sta     $FF02,y

        ;  Second */
        clc
        ;  cb2l_off1e:   lda     $FF01,y
cb1l_off1i:
        adc     $FF01,y
        tax
cb2h_off1e:
        lda     $FF02,y
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
        sta     $FF01,y
        txa
        ror     a
cb2l_off0c:
        sta     $FF01,y

        lda     rept
        and     #1
        beq     rep_even

        ;  tk = getbithuff(8) << 4;
        jsr     _getdatahuff
        lda     _shiftl4_h,x
        sta     tmp2
        lda     _shiftl4_l,x
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
        adc     $FF01,y
cb2l_off0e:
        sta     $FF01,y
        lda     tmp2
cb2h_off0d:
        adc     $FF01,y
cb2h_off0e:
        sta     $FF01,y

        txa
cb2l_off1f:
        adc     $FF02,y
cb2l_off1g:
        sta     $FF02,y
        lda     tmp2
cb2h_off1f:
        adc     $FF02,y
cb2h_off1g:
        sta     $FF02,y

rep_even:
        ldx     rept
        inx
        cpx     rep_loop
        beq     rep_loop_done
        stx     rept
        lda     col
        beq     :+
        jmp     do_rep_loop
:       ldx     colh
        beq     finish_col_loop
        tay     ;  fix ZERO flag needed at do_rep_loop
        jmp     do_rep_loop
rep_loop_done:
        lda     nreps
        cmp     #9
        bne     nine_reps_loop_done
        jmp     nine_reps_loop
nine_reps_loop_done:

        lda     col               ; is col loop done?
        beq     :+
        jmp     col_loop1
:       ldx     colh
        beq     finish_col_loop
        jmp     col_loop1

finish_col_loop:
        clc
        jsr     _copy_data
        dec     repeats
        bmi     r_loop_done
        jmp     r_loop
r_loop_done:

        clc
        lda     _row_idx
        adc     #<(WIDTH*2)
        sta     _row_idx
        lda     _row_idx+1
        adc     #>(WIDTH*2)
        sta     _row_idx+1

        lda     _row_idx_plus2
        adc     #<(WIDTH*2)
        sta     _row_idx_plus2
        lda     _row_idx_plus2+1
        adc     #>(WIDTH*2)
        sta     _row_idx_plus2+1

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
last_dyndiv:    .res 1
