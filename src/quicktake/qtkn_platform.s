        .export       _consume_extra
        .export       _init_row
        .export       _decode_row

        .export       _init_top
        .export       _init_shiftl4
        .export       _init_next_line
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
        .import       _next_line_l, _next_line_h
        .import       _div48_l
        .import       _dyndiv_l
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

; ZP vars share locations when they can - they're usually limited to one function
val0             = _zp2
wordcnt          = _zp2
genptr           = _zp2
_x               = _zp2

val1             = _zp4
src_idx          = _zp4

val              = _zp6
rep_loop         = _zp6

col              = _zp7
rept             = _zp13

.segment "LC"

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

.proc _init_next_line
        lda     #<_next_line_l
        ldx     #>_next_line_l
        jsr     pushax

        lda     #<2048
        jsr     pusha0

        lda     #<USEFUL_DATABUF_SIZE
        ldx     #>USEFUL_DATABUF_SIZE
        jsr     _memset

        lda     #<_next_line_h
        ldx     #>_next_line_h
        jsr     pushax

        lda     #>2048
        jsr     pusha0

        lda     #<USEFUL_DATABUF_SIZE
        ldx     #>USEFUL_DATABUF_SIZE
        jmp     _memset
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
        sta     genptr+1
        lda     #<_src
        sta     genptr
ctrl_huff_next:
        ldy     src_idx

        lda     (genptr),y
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
        lda     (genptr),y          ; A = value

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
        inc     genptr+1

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

        lda     (genptr),y
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
        lda     (genptr),y          ; A = value

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
        inc     genptr+1

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
        ldy     tree
        ldx     tree_huff_ctrl_map,y
        stx     _huff_numc
        inx
        stx     _huff_numc_h

        jsr     _getctrlhuff
        sta     tree

        beq     tree_zero_2

tree_not_zero_2:
        dec     col

        cmp     #8
        bne     norm_huff

        jsr     _discarddatahuff8
        jsr     _discarddatahuff8
        jsr     _discarddatahuff8
        jsr     _discarddatahuff8

        lda     col
        bne     col_loop2
        jmp     check_c_loop

norm_huff:
        adc     #>(_huff_data+256)
        sta     _huff_numdd

        jsr     _discarddatahuff
        jsr     _discarddatahuff
        jsr     _discarddatahuff
        jsr     _discarddatahuff

        lda     col
        bne     col_loop2
        jmp     check_c_loop

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
        bcc     :+
        ldx     #8
:       stx     rep_loop

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
        beq     :+
        jsr     _discarddatahuff

:       ldx     rept
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

; Expects genptr to point to table
; factor in A
.proc _init_divtable
        sta     abck
        stx     xbck
        sty     ybck
        sta     fact+1

        lda     #0
        sta     wordcnt

:       ldx     wordcnt
        lda     #$80
.ifndef APPROX_DIVISION
        jsr     pushax
fact:   lda     #$FF
        jsr     tosdiva0
.else
fact:   ldy     #$FF
        jsr     approx_div16x8_direct
.endif
        ldy     wordcnt
        bmi     overflows_neg     ; stop if signed < 0
build_table_n:
        sta     $FF00,y
        txa
        bne     overflows         ; Stop if result > 256
        inc     wordcnt
        bne     :-
        jmp     done

overflows:                        ; Fill the rest
        lda     #$FF
build_table_o:
:       sta     $FF00,y
        iny
        bmi     overflows_neg
        bne     :-

overflows_neg:
        lda     #$00
build_table_u:
:       sta     $FF00,y
        iny
        bne     :-

done:
abck = *+1
        lda     #$FF
xbck = *+1
        ldx     #$FF
ybck = *+1
        ldy     #$FF

        rts
.endproc

.proc _init_top
        jsr     _init_huff
        jsr     _init_shiftl4
        ldx     #>_div48_l
        stx     _init_divtable::build_table_n+2
        stx     _init_divtable::build_table_o+2
        stx     _init_divtable::build_table_u+2
        lda     #48
        jsr     _init_divtable
        jmp     _init_next_line
.endproc

.macro SET_BUF_TREE_EIGHT mult_factor, address, low_label, high_label
        jsr     _getdatahuff8
mult_factor:
        ldx     #$FF
        jsr     mult8x8r16_direct
        ldy     col
low_label: 
        sta     address,y
        txa
high_label: 
        sta     address,y
.endmacro

.macro SET_VAL_TREE_EIGHT mult_factor, val
        jsr     _getdatahuff8
mult_factor:
        ldx     #$FF
        jsr     mult8x8r16_direct
        ldy     col
        sta     val
        stx     val+1
.endmacro


.macro INTERPOLATE_BUF_TOKEN val_a, addr2, l2, h2, val_b, addr_res, res_l, res_h, token
        clc
        lda     val_a
l2:     adc     addr2,y
        tax
        lda     val_a+1
h2:     adc     addr2,y
        lsr     a
        sta     tmp1
        txa
        ror     a

        clc
        adc     val_b
        tax
        lda     tmp1
        adc     val_b+1
        lsr     a

        .ifblank token
res_h:  sta     addr_res,y
        sta     tmp1
        txa
        ror     a
res_l:  sta     addr_res,y
        .else
        sta     tmp1          ; High byte in tmp1
        txa
        ror     a             ; Low byte in A

token:  ldx     #$FF
        clc
        adc     _shiftl4_l,x
res_l:  sta     addr_res,y
        lda     tmp1
        adc     _shiftl4_h,x
res_h:  sta     addr_res,y
        .endif
.endmacro

.macro INTERPOLATE_VAL_TOKEN val, addr2, l2, h2, addr3, l3, h3, res, token
        clc
        lda     val
l2:     adc     addr2,y
        tax
        lda     val+1
h2:     adc     addr2,y
        lsr     a
        sta     tmp1
        txa
        ror     a

        clc
l3:     adc     addr3,y
        tax
        lda     tmp1
h3:     adc     addr3,y
        lsr     a

        .ifblank token
        sta     res+1
        sta     tmp1
        txa
        ror     a
        sta     res
        .else
        sta     tmp1          ; High byte in tmp1
        txa
        ror     a             ; Low byte in A

token:  ldx     #$FF
        clc
        adc     _shiftl4_l,x
        sta     res
        lda     tmp1
        adc     _shiftl4_h,x
        sta     res+1
        .endif
.endmacro

.macro INCR_BUF_TOKEN addr1, l1, h1, addr2, l2, h2, token
        clc
.ifnblank token
        ldx     token
.endif
        lda     _shiftl4_l,x
l1:     adc     addr1,y
l2:     sta     addr2,y
        lda     _shiftl4_h,x
h1:     adc     addr1,y
h2:     sta     addr2,y
.endmacro

.macro INCR_VAL_TOKEN val, token
        clc
.ifnblank token
        ldx     token
.endif
        lda     _shiftl4_l,x
        adc     val
        sta     val
        lda     _shiftl4_h,x
        adc     val+1
        sta     val+1
.endmacro

.proc _decode_row
        lda     #1
        sta     repeats
r_loop:
        lda     _row_idx
        ldx     _row_idx+1
        ldy     repeats
        ;  logic inverted from C because here we dec R */
        bne     store_set
store_plus_2:
        lda     _row_idx_plus2
        ldx     _row_idx_plus2+1

store_set:
        tay
        sty     dest0a+1
        sty     dest0b+1
        sty     dest0c+1
        sty     dest0d+1
        inx                       ; Start at page 2
        stx     dest0a+2
        stx     dest0b+2
        stx     dest0c+2
        stx     dest0d+2

        iny
        bne     :+
        inx
:       sty     dest1a+1
        sty     dest1b+1
        sty     dest1c+1
        sty     dest1d+1
        stx     dest1a+2
        stx     dest1b+2
        stx     dest1c+2
        stx     dest1d+2

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
        ror     a

        ; next_line[WIDTH+1] = factor<<7
        stx     _next_line_h+(WIDTH+1)
        stx     val0+1
        sta     _next_line_l+(WIDTH+1)
        sta     val0

        ; tree = 1,
        lda     #1
        sta     tree

        ; col = WIDTH
        lda     #<(WIDTH)
        sta     col
        ldx     #>(WIDTH)
        stx     colh

        ; Init the numerous patched locations
        ; Worth the ugliness as there are 44
        ; locations, and we run that path
        ; 320*120*2 times so doing $nnnn,y
        ; instead of ($nn), y we spare:
        ; (320*120*2)*(2*44): 7 SECONDS
        ; (and spend only 0.165s shifting
        ; the high bytes, only 120*2 times)
        lda     #>(_next_line_h+256)
        sta     next0ha+2
        sta     next0hb+2

        sta     next1ha+2
        sta     next1hb+2
        sta     next1hc+2
        sta     next1hd+2
        sta     next1he+2
        sta     next1hf+2
        sta     next1hg+2
        sta     next1hh+2
        sta     next1hi+2

        sta     next2ha+2
        sta     next2hb+2
        sta     next2hc+2
        sta     next2hd+2
        sta     next2he+2
        sta     next2hf+2
        sta     next2hg+2
        sta     next2hh+2
        sta     next2hi+2

        sta     next3ha+2
        sta     next3hb+2

        lda     #>(_next_line_l+256)
        sta     next0la+2
        sta     next0lb+2

        sta     next1la+2
        sta     next1lb+2
        sta     next1lc+2
        sta     next1ld+2
        sta     next1le+2
        sta     next1lf+2
        sta     next1lg+2
        sta     next1lh+2
        sta     next1li+2

        sta     next2la+2
        sta     next2lb+2
        sta     next2lc+2
        sta     next2ld+2
        sta     next2le+2
        sta     next2lf+2
        sta     next2lg+2
        sta     next2lh+2
        sta     next2li+2

        sta     next3la+2
        sta     next3lb+2

col_loop1:
        ; 320 loop, repeated twice, function
        ; called 120 times: every line here
        ; costs 153-400ms depending on the
        ; number of cycles.

        ldy     tree
        ldx     tree_huff_ctrl_map,y
        stx     _huff_numc
        inx
        stx     _huff_numc_h

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
        SET_VAL_TREE_EIGHT mult_factor1, val1
divt1a: lda     _div48_l,x
dest1a: sta     $FFFF,y
        SET_VAL_TREE_EIGHT mult_factor2, val0
divt0a: lda     _div48_l,x
dest0a: sta     $FFFF,y

        SET_BUF_TREE_EIGHT mult_factor3, $FF02, next2la, next2ha
        SET_BUF_TREE_EIGHT mult_factor4, $FF01, next1la, next1ha

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

        jsr     _getdatahuff
        stx     tk1+1
        jsr     _getdatahuff
        stx     tk2+1
        jsr     _getdatahuff
        stx     tk3+1
        jsr     _getdatahuff
        stx     tk4+1

        ldy     col           ; Reload Y

        INTERPOLATE_VAL_TOKEN val0, $FF02, next2lb, next2hb, $FF01, next1lb, next1hb, val1, tk1
        tax
divt1b: lda     _div48_l,x
dest1b: sta     $FFFF,y

        INTERPOLATE_BUF_TOKEN val0, $FF03, next3la, next3ha, val1, $FF02, next2lc, next2hc, tk3

        INTERPOLATE_VAL_TOKEN val1, $FF01, next1lc, next1hc, $FF00, next0la, next0ha, val0, tk2
        tax
divt0b: lda     _div48_l,x
dest0b: sta     $FFFF,y

        INTERPOLATE_BUF_TOKEN val1, $FF02, next2ld, next2hd, val0, $FF01, next1ld, next1hd, tk4

        tya                   ; is col loop done?
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
        bne     declow2
        jsr     dec_buf_pages
declow2:
        sec
        sbc     #2
        sta     col
        tay

        INTERPOLATE_VAL_TOKEN val0, $FF02, next2le, next2he, $FF01, next1le, next1he, val1
        ldx     tmp1
divt1c: lda     _div48_l,x
dest1c: sta     $FFFF,y

        INTERPOLATE_BUF_TOKEN val0, $FF03, next3lb, next3hb, val1, $FF02, next2lf, next2hf

        INTERPOLATE_VAL_TOKEN val1, $FF01, next1lf, next1hf, $FF00, next0lb, next0hb, val0
        ldx     tmp1
divt0c: lda     _div48_l,x
dest0c: sta     $FFFF,y

        INTERPOLATE_BUF_TOKEN val1, $FF02, next2lg, next2hg, val0, $FF01, next1lg, next1hg

        lda     rept
        and     #1
        beq     rep_even

        ; tk = getbithuff(8) << 4;
        jsr     _getdatahuff
        stx     tmp1

        ldy     col
        ; Increment values by token (in tmp2/X)
        INCR_VAL_TOKEN val1
        tax
divt1d: lda     _div48_l,x
dest1d: sta     $FFFF,y

        INCR_VAL_TOKEN val0, tmp1
        tax
divt0d: lda     _div48_l,x
dest0d: sta     $FFFF,y

        INCR_BUF_TOKEN $FF02, next2lh, next2hh, $FF02, next2li, next2hi, tmp1
        INCR_BUF_TOKEN $FF01, next1lh, next1hh, $FF01, next1li, next1hi

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
        dec     repeats
        bmi     r_loop_done
        jmp     r_loop
r_loop_done:

        ; Advance rows
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

.proc _init_row
        ldx     #>(_next_line_l+256)      ; start at second page
        stx     load_next_lf+2
        stx     load_next_ls+2
        stx     store_next_lf+2
        stx     store_next_ls+2
        ldx     #>(_next_line_h+256)
        stx     load_next_hf+2
        stx     load_next_hs+2
        stx     store_next_hf+2
        stx     store_next_hs+2

        ldx     #>_div48_l
        stx     _decode_row::divt0a+2
        stx     _decode_row::divt0b+2
        stx     _decode_row::divt0c+2
        stx     _decode_row::divt0d+2
        stx     _decode_row::divt1a+2
        stx     _decode_row::divt1b+2
        stx     _decode_row::divt1c+2
        stx     _decode_row::divt1d+2

        ldy     _last               ; is last 8bits?
        cpy     #18
        bcs     small_val

        lda     _val_from_last,y    ; no, 16x8 mult
        ldx     _val_hi_from_last,y
        jsr     pushax
        lda     _factor
        sta     _last

        cmp     #48
        beq     :+
        ldx     #>_dyndiv_l
        stx     _init_divtable::build_table_n+2
        stx     _init_divtable::build_table_o+2
        stx     _init_divtable::build_table_u+2
        stx     _decode_row::divt0a+2
        stx     _decode_row::divt0b+2
        stx     _decode_row::divt0c+2
        stx     _decode_row::divt0d+2
        stx     _decode_row::divt1a+2
        stx     _decode_row::divt1b+2
        stx     _decode_row::divt1c+2
        stx     _decode_row::divt1d+2
        cmp     last_dyndiv
        beq     :+
        sta     last_dyndiv
        jsr     _init_divtable        ; Init current factor division table

:       jsr     tosmula0
        jmp     check_multiplier

small_val:                          ; Last is 8bit, do a small mult
        ldx     _factor
        stx     _last
        cpx     #48
        beq     :+
        lda     #>_dyndiv_l
        sta     _init_divtable::build_table_n+2
        sta     _init_divtable::build_table_o+2
        sta     _init_divtable::build_table_u+2
        sta     _decode_row::divt0a+2
        sta     _decode_row::divt0b+2
        sta     _decode_row::divt0c+2
        sta     _decode_row::divt0d+2
        sta     _decode_row::divt1a+2
        sta     _decode_row::divt1b+2
        sta     _decode_row::divt1c+2
        sta     _decode_row::divt1d+2
        cpx     last_dyndiv
        beq     :+
        txa
        sta     last_dyndiv
        jsr     _init_divtable

:       lda     _val_from_last,y    ; yes, 8x8 mult
        jsr     mult8x8r16_direct

check_multiplier:
        cpx     #$0F                ; is multiplier 0xFF?
        bne     check_0x100         ; Check before shifting
        cmp     #$F0
        bcc     check_0x100

mult_0xFF:
        ldy     #<USEFUL_DATABUF_SIZE
        lda     #>USEFUL_DATABUF_SIZE
        sta     wordcnt+1

setup_curbuf_x_ff:
        ; load
        dey
        sty     wordcnt
load_next_hf:
        ldx     $FF00,y
load_next_lf:
        lda     $FF00,y
        ; tmp32 in AX
        stx     tmp1                ; Store to subtract
        sec
        sbc     tmp1                ; subtract high word from low : -(tmp>>8)
        bcs     store_next_lf
        dex

store_next_lf:
        sta     $FF00,y
        txa
store_next_hf:
        sta     $FF00,y

        cpy     #0
        bne     setup_curbuf_x_ff
        dec     load_next_hf+2
        dec     load_next_lf+2
        dec     store_next_hf+2
        dec     store_next_lf+2
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
        sta     ptr2                ; multiplier stored in ptr2/+1

slow_mults:                         ; and multiply
        ldy     #<USEFUL_DATABUF_SIZE
        sty     wordcnt
        lda     #>USEFUL_DATABUF_SIZE
        sta     wordcnt+1

setup_curbuf_x_slow:
        ; load
        dey
        sty     wordcnt
load_next_hs:
        ldx     $FF00,y
load_next_ls:
        lda     $FF00,y

        cpx     #$80
        bcc     posmult
        ; reverse sign, less expensive than extending
        ; and doing a 16x24 mult
        clc
        eor     #$FF
        adc     #1
        pha
        txa
        eor     #$FF
        adc     #0
        tax
        pla
        ; multiply
        jsr     mult16x16mid16_direct
        clc
        adc     #1
        clc
        eor     #$FF
        adc     #1
        pha
        txa
        eor     #$FF
        adc     #0
        tax
        pla
        jmp     set0
posmult:
        jsr     mult16x16mid16_direct

set0:
        ldy     wordcnt
store_next_ls:
        sta     $FF00,y
        txa
store_next_hs:
        sta     $FF00,y

        cpy     #0
        bne     setup_curbuf_x_slow
        dec     load_next_hs+2
        dec     load_next_ls+2
        dec     store_next_hs+2
        dec     store_next_ls+2
        dec     wordcnt+1
        bpl     setup_curbuf_x_slow
init_done:
        rts
.endproc

.segment "LC"

.proc dec_buf_pages
        dec     colh
        dec     _decode_row::dest0a+2
        dec     _decode_row::dest0b+2
        dec     _decode_row::dest0c+2
        dec     _decode_row::dest0d+2
        dec     _decode_row::dest1a+2
        dec     _decode_row::dest1b+2
        dec     _decode_row::dest1c+2
        dec     _decode_row::dest1d+2

        dec     _decode_row::next0ha+2
        dec     _decode_row::next0la+2
        dec     _decode_row::next0hb+2
        dec     _decode_row::next0lb+2

        dec     _decode_row::next1ha+2
        dec     _decode_row::next1la+2
        dec     _decode_row::next1hb+2
        dec     _decode_row::next1lb+2
        dec     _decode_row::next1hc+2
        dec     _decode_row::next1lc+2
        dec     _decode_row::next1hd+2
        dec     _decode_row::next1ld+2
        dec     _decode_row::next1he+2
        dec     _decode_row::next1le+2
        dec     _decode_row::next1hf+2
        dec     _decode_row::next1lf+2
        dec     _decode_row::next1hg+2
        dec     _decode_row::next1lg+2
        dec     _decode_row::next1hh+2
        dec     _decode_row::next1lh+2
        dec     _decode_row::next1hi+2
        dec     _decode_row::next1li+2

        dec     _decode_row::next2ha+2
        dec     _decode_row::next2la+2
        dec     _decode_row::next2hb+2
        dec     _decode_row::next2lb+2
        dec     _decode_row::next2hc+2
        dec     _decode_row::next2lc+2
        dec     _decode_row::next2hd+2
        dec     _decode_row::next2ld+2
        dec     _decode_row::next2he+2
        dec     _decode_row::next2le+2
        dec     _decode_row::next2hf+2
        dec     _decode_row::next2lf+2
        dec     _decode_row::next2hg+2
        dec     _decode_row::next2lg+2
        dec     _decode_row::next2hh+2
        dec     _decode_row::next2lh+2
        dec     _decode_row::next2hi+2
        dec     _decode_row::next2li+2

        dec     _decode_row::next3ha+2
        dec     _decode_row::next3la+2
        dec     _decode_row::next3hb+2
        dec     _decode_row::next3lb+2
        rts
.endproc

.segment "BSS"

tree:           .res 1
nreps:          .res 1
colh:           .res 1
repeats:        .res 1
incr:           .res 1
code:           .res 1
last_dyndiv:    .res 1

.segment "DATA"

tree_huff_ctrl_map:
        .repeat 9,I
        .byte   (I*2) + >_huff_ctrl
        .endrep
