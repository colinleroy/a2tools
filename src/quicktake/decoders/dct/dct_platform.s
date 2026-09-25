        .export _get_coeffs, _cache_read
        .export _advance_block
        .export _idct_common, _idct_1d_rows, _idct_1d_cols
        .export _shift_table, _bits_table
        .export _init_idx, _update_idx

        .import _mul362_h, _mul362_m, _mul362_l
        .import _mul473_h, _mul473_m, _mul473_l
        .import _mul277_h, _mul277_m, _mul277_l
        .import _mul669_h, _mul669_m, _mul669_l

        .import _tmp0, _tmp1, _tmp2, _tmp3, _tmp4, _tmp5, _tmp6, _tmp7
        .import _tmp10, _tmp11, _tmp12, _tmp13
        .import _z5, _z10, _z11, _z12, _z13
        .import _coef, _row_out, _raw_image

        .import _cache
        .import _bitmask, _negate, _ign_bits
        .import _SCAN, _scan
        .import _nbits_avail, _numbits

        .import _blocks_per_row, _blocks_rem_in_row
        .import _actual_width

        .import _ifd, _cache_start
        .import _read, _cputsxy
        .import decsp4, pushax 
        .importzp _prev_ram_irq_vector, c_sp
        .importzp xbck, ybck, abck

cur_cache_ptr     = _prev_ram_irq_vector ; Cache pointer, 2-bytes

; Not a convenience to change. We want 1 for 7-bits precision
DESCALE_FACTOR = 1

CACHE_END = _cache + CACHE_SIZE
.assert <CACHE_END = 0, error

        .segment "CODE"

; int8 * x => >> 8 => (int8)
.macro do_mul TABL, TABM;, TABH
.scope
        bmi     neg
        tay
        lda     TABM,y
        ; ldx     TABH,y
        jmp     done

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
done:
.endscope
.endmacro

.macro MULT_362
        do_mul _mul362_l, _mul362_m ;, _mul_362_h
.endmacro

.macro MULT_473
        do_mul _mul473_l, _mul473_m ;, _mul_473_h
.endmacro

.macro MULT_277
        do_mul _mul277_l, _mul277_m; , _mul_277_h
.endmacro

.macro MULT_669
        do_mul _mul669_l, _mul669_m;, _mul_669_h
.endmacro

_reading_str: .byte          "Reading     ", $0D, $0A, $00
_decoding_str:.byte          "Decoding    ", $0D, $0A, $00

.proc fill_cache
        stx     xbck
        sty     ybck
        ldx     #0
        lda     #7
        jsr     pushax
        lda     #<_reading_str
        ldx     #>_reading_str
        jsr     _cputsxy

        ; Push read fd
        jsr     decsp4
        ldy     #$03

        lda     #$00                    ; ifd is never going to be > 255
        sta     (c_sp),y
        dey
        lda     _ifd
        sta     (c_sp),y
        dey

        ; Push buffer
        lda     _cache_start+1
        sta     _cache_read+1
        sta     (c_sp),y
        dey

        lda     _cache_start
        sta     _cache_read
        sta     (c_sp),y

        ; Push count (CACHE_SIZE)
        lda     #<CACHE_SIZE
        ldx     #>CACHE_SIZE
        jsr     _read

        ldx     #0
        lda     #7
        jsr     pushax
        lda     #<_decoding_str
        ldx     #>_decoding_str
        jsr     _cputsxy
        ldx     xbck
        ldy     ybck
        jmp     inc_cache_finish
.endproc

.proc inc_cache_high
        inc     _cache_read+1
        lda     _cache_read+1
        cmp     #>CACHE_END
        bne     inc_cache_finish
        jmp     fill_cache
.endproc

.proc inc_cache
        sta     abck
        lda     #7
        sta     _nbits_avail
        inc     _cache_read
        beq     inc_cache_high
.endproc
        ; Fallthrough
.proc inc_cache_finish
        lda     abck
        jmp     inc_cache_done
.endproc
; Much left to optimize there
; Split read to avoid lda/ora/sta *2
; use ZP

; Exits with carry set if last bit set (neg)
; Exits with bitval low in A, bitpos in X
.macro GET_BITVAL
        lda     #0                      ; Init bitval
        tax                             ; bitpos is X

        ldy     _numbits                ; Get all bits
next_bit:
        dec     _nbits_avail
        bmi     inc_cache
inc_cache_done:
_cache_read = *+1
        lsr     $FFFF
        dec     _ign_bits
        bpl     bit_done
        bcc     :+
        ora     _bitmask,x
:       inx                             ; update bitpos
bit_done:
        dey
        bne     next_bit

        ldy     _scan                   ; is coef negative?
        beq     store_coef
        bcc     store_coef

        ora     _negate,x               ; extend sign bit
.endmacro

.proc _get_coeffs
        ldy     #0
next_coeff:
        sty     _scan
        ldx     _SCAN,y

bits_table = *+1
        lda     $FFFF,y                 ; get numbits
        beq     inc_scan                ; eq jmp here
load_coef:
        sta     _numbits
shift_table = *+1
        lda     $FFFF,y                 ; get ignored bits shift
        sta     _ign_bits

        GET_BITVAL                      ; Exits with carry if neg, low byte in A

store_coef:
        ; coef[r] = (int8)(bitval);
        ldx     _SCAN,y
inc_scan:
        sta     _coef,x                 ; zero, easy way out
        iny
        cpy     #64
        bcc     next_coeff
        rts
.endproc
_bits_table = _get_coeffs::bits_table
_shift_table = _get_coeffs::shift_table
inc_cache_done = _get_coeffs::inc_cache_done
_cache_read = _get_coeffs::_cache_read

; Fixme lots to optimize
; Move block increment to a single-byte var and use it as index in idct_1d_cols
; Move row increment to LUT-based (always lands at $XX00)
block_step:     .byte 16, 8
row_step_l:     .byte <(8*RAW_WIDTH-DECODE_WIDTH+16), <(4*RAW_WIDTH-DECODE_WIDTH+8)
row_step_h:     .byte >(8*RAW_WIDTH-DECODE_WIDTH+16), >(4*RAW_WIDTH-DECODE_WIDTH+8)
.proc _advance_block
        dec     _blocks_rem_in_row
        beq     inc_row
inc_block:
        ldx     _actual_width+1
        lda     block_step,x
        clc
        adc     idx0_1+1
        sta     idx0_1+1
        lda     idx0_1+2
        adc     #0
        sta     idx0_1+2
        jmp     _update_idx
inc_row:
        ldx     _actual_width+1
        lda     row_step_l,x
        clc
        adc     idx0_1+1
        sta     idx0_1+1
        lda     row_step_h,x
        adc     idx0_1+2
        sta     idx0_1+2
        lda     _blocks_per_row
        sta     _blocks_rem_in_row
        jmp     _update_idx
.endproc

.proc _idct_common
        lda     _tmp10
        clc
        adc     _tmp13
        sta     _tmp0

        lda     _tmp10
        sec
        sbc     _tmp13
        sta     _tmp3

        lda     _tmp12
        MULT_362
        sec
        sbc     _tmp13
        sta     _tmp12

        clc
        adc     _tmp11
        sta     _tmp1

        lda     _tmp11
        sec
        sbc     _tmp12
        sta     _tmp2

        lda     _z11
        clc
        adc     _z13
        sta     _tmp7

        lda     _z11
        sec
        sbc     _z13
        MULT_362
        sta     _tmp11

        lda     _z10
        MULT_669
        sta     _z13

        lda     _z10
        clc
        adc     _z12
        MULT_473
        sta     _z5

        sec
        sbc     _z13
        sta     _tmp12

        lda     _z12
        MULT_277
        sec
        sbc     _z5
        sta     _tmp10

        lda     _tmp12
        sec
        sbc     _tmp7
        sta     _tmp6

        lda     _tmp11
        sec
        sbc     _tmp6
        sta     _tmp5

        clc
        adc     _tmp10
        sta     _tmp4

        rts
.endproc

.proc _idct_1d_rows
        lda     #0
next_y:
        tay
        lda     _coef+2,y
        ora     _coef+4,y
        ora     _coef+6,y
        ora     _coef+8,y
        ora     _coef+10,y
        ora     _coef+12,y
        ora     _coef+14,y
        bne     full_rows

        lda     _coef+0,y               ; Easy way, all AC = 0
        sta     _row_out+0,y
        sta     _row_out+2,y
        sta     _row_out+4,y
        sta     _row_out+6,y
        sta     _row_out+8,y
        sta     _row_out+10,y
        sta     _row_out+12,y
        sta     _row_out+14,y

        tya
        clc
        adc     #16
        bmi     :+                      ; > 128
        jmp     next_y
:       rts

full_rows:
        lda     _coef+0,y
        clc
        adc     _coef+8,y
        sta     _tmp10
        lda     _coef+0,y
        sec
        sbc     _coef+8,y
        sta     _tmp11

        lda     _coef+4,y
        sec
        sbc     _coef+12,y
        sta     _tmp12
        lda     _coef+4,y
        clc
        adc     _coef+12,y
        sta     _tmp13

        lda     _coef+10,y
        sec
        sbc     _coef+6,y
        sta     _z10

        lda     _coef+2,y
        clc
        adc     _coef+14,y
        sta     _z11

        lda     _coef+2,y
        sec
        sbc     _coef+14,y
        sta     _z12

        lda     _coef+10,y
        clc
        adc     _coef+6,y
        sta     _z13

        sty     ybck
        jsr     _idct_common
        ldy     ybck

        lda     _tmp0
        clc
        adc     _tmp7
        sta     _row_out+0,y

        lda     _tmp1
        clc
        adc     _tmp6
        sta     _row_out+2,y

        lda     _tmp2
        clc
        adc     _tmp5
        sta     _row_out+4,y

        lda     _tmp3
        sec
        sbc     _tmp4
        sta     _row_out+6,y

        lda     _tmp3
        clc
        adc     _tmp4
        sta     _row_out+8,y

        lda     _tmp2
        sec
        sbc     _tmp5
        sta     _row_out+10,y

        lda     _tmp1
        sec
        sbc     _tmp6
        sta     _row_out+12,y

        lda     _tmp0
        sec
        sbc     _tmp7
        sta     _row_out+14,y

        tya
        clc
        adc     #16
        bmi     :+                      ; > 128
        jmp     next_y
:       rts
.endproc

.macro CLAMPU
.scope
        asl
        bcc     :+
        lda     #$FF
:
.endscope
.endmacro

_idct_1d_cols:
        ldx     #0
next_x:
        lda     _row_out+16,x
        ora     _row_out+32,x
        ora     _row_out+48,x
        ora     _row_out+64,x
        ora     _row_out+80,x
        ora     _row_out+96,x
        ora     _row_out+112,x
        bne     full_cols

        lda     _actual_width
        cmp     #<160
        bne     fast_cols_scale_down

        lda     _row_out+0,x            ; Easy way out, 160w images
        CLAMPU
idx0_1: sta     $FFFF,x
idx1_1: sta     $FFFF,x
idx2_1: sta     $FFFF,x
idx3_1: sta     $FFFF,x
idx4_1: sta     $FFFF,x
idx5_1: sta     $FFFF,x
idx6_1: sta     $FFFF,x
idx7_1: sta     $FFFF,x
        inx
idx0_2: sta     $FFFF,x
idx1_2: sta     $FFFF,x
idx2_2: sta     $FFFF,x
idx3_2: sta     $FFFF,x
idx4_2: sta     $FFFF,x
idx5_2: sta     $FFFF,x
idx6_2: sta     $FFFF,x
idx7_2: sta     $FFFF,x
        inx
        cpx     #16
        bcs     :+
        jmp     next_x
:       rts

fast_cols_scale_down:
        txa
        lsr
        tay
        lda     _row_out+0,x            ; Easy way out, 320w images
        CLAMPU
idx0_3: sta     $FFFF,y
idx1_3: sta     $FFFF,y
idx2_3: sta     $FFFF,y
idx3_3: sta     $FFFF,y
        inx
        inx
        cpx     #16
        bcs     :+
        jmp     next_x
:       rts

full_cols:

        lda     _row_out+0,x
        clc
        adc     _row_out+64,x
        sta     _tmp10

        lda     _row_out+0,x
        sec
        sbc     _row_out+64,x
        sta     _tmp11

        lda     _row_out+32,x
        sec
        sbc     _row_out+96,x
        sta     _tmp12

        lda     _row_out+32,x
        clc
        adc     _row_out+96,x
        sta     _tmp13

        lda     _row_out+80,x
        sec
        sbc     _row_out+48,x
        sta     _z10

        lda     _row_out+16,x
        clc
        adc     _row_out+112,x
        sta     _z11

        lda     _row_out+16,x
        sec
        sbc     _row_out+112,x
        sta     _z12

        lda     _row_out+80,x
        clc
        adc     _row_out+48,x
        sta     _z13

        stx     xbck
        jsr     _idct_common
        ldx     xbck

        lda     _actual_width
        cmp     #<160
        beq     full_cols_no_scale
        jmp     full_cols_scale_down

full_cols_no_scale:
        txa
        tay
        iny                             ; Y = X+1

        lda     _tmp0
        clc
        adc     _tmp7
        CLAMPU
idx0_4: sta     $FFFF,x
idx0_5: sta     $FFFF,y

        lda     _tmp2
        clc
        adc     _tmp5
        CLAMPU
idx2_4: sta     $FFFF,x
idx2_5: sta     $FFFF,y

        lda     _tmp3
        clc
        adc     _tmp4
        CLAMPU
idx4_4: sta     $FFFF,x
idx4_5: sta     $FFFF,y

        lda     _tmp1
        sec
        sbc     _tmp6
        CLAMPU
idx6_4: sta     $FFFF,x
idx6_5: sta     $FFFF,y

        lda     _tmp1
        clc
        adc     _tmp6
        CLAMPU
idx1_4: sta     $FFFF,x
idx1_5: sta     $FFFF,y

        lda     _tmp3
        sec
        sbc     _tmp4
        CLAMPU
idx3_4: sta     $FFFF,x
idx3_5: sta     $FFFF,y

        lda     _tmp2
        sec
        sbc     _tmp5
        CLAMPU
idx5_4: sta     $FFFF,x
idx5_5: sta     $FFFF,y

        lda     _tmp0
        sec
        sbc     _tmp7
        CLAMPU
idx7_4: sta     $FFFF,x
idx7_5: sta     $FFFF,y

        inx
        inx
        cpx     #16
        bcs     :+
        jmp     next_x
:       rts

full_cols_scale_down:
        txa
        lsr
        tay

        lda     _tmp0
        clc
        adc     _tmp7
        CLAMPU
idx0_6: sta     $FFFF,y

        lda     _tmp2
        clc
        adc     _tmp5
        CLAMPU
idx1_6: sta     $FFFF,y

        lda     _tmp3
        clc
        adc     _tmp4
        CLAMPU
idx2_6: sta     $FFFF,y

        lda     _tmp1
        sec
        sbc     _tmp6
        CLAMPU
idx3_6: sta     $FFFF,y

        inx
        inx
        cpx     #16
        bcs     :+
        jmp     next_x
:       rts

.proc _init_idx
        ldy     #>_raw_image
        sty     idx0_1+2

        .assert <_raw_image = 0, error
        ldy     #<_raw_image
        sty     idx0_1+1
        rts
.endproc

.proc _update_idx
        ldy     idx0_1+2
        sty     idx0_2+2
        sty     idx0_3+2
        sty     idx0_4+2
        sty     idx0_5+2
        sty     idx0_6+2
        .assert RAW_WIDTH = 512, error
        iny
        iny
        sty     idx1_1+2
        sty     idx1_2+2
        sty     idx1_3+2
        sty     idx1_4+2
        sty     idx1_5+2
        sty     idx1_6+2

        iny
        iny
        sty     idx2_1+2
        sty     idx2_2+2
        sty     idx2_3+2
        sty     idx2_4+2
        sty     idx2_5+2
        sty     idx2_6+2

        iny
        iny
        sty     idx3_1+2
        sty     idx3_2+2
        sty     idx3_3+2
        sty     idx3_4+2
        sty     idx3_5+2
        sty     idx3_6+2

        iny
        iny
        sty     idx4_1+2
        sty     idx4_2+2
        sty     idx4_4+2
        sty     idx4_5+2

        iny
        iny
        sty     idx5_1+2
        sty     idx5_2+2
        sty     idx5_4+2
        sty     idx5_5+2

        iny
        iny
        sty     idx6_1+2
        sty     idx6_2+2
        sty     idx6_4+2
        sty     idx6_5+2

        iny
        iny
        sty     idx7_1+2
        sty     idx7_2+2
        sty     idx7_4+2
        sty     idx7_5+2

        .assert <_raw_image = 0, error
        ldy     idx0_1+1
        sty     idx0_2+1
        sty     idx0_3+1
        sty     idx0_4+1
        sty     idx0_5+1
        sty     idx0_6+1

        sty     idx1_1+1
        sty     idx1_2+1
        sty     idx1_3+1
        sty     idx1_4+1
        sty     idx1_5+1
        sty     idx1_6+1

        sty     idx2_1+1
        sty     idx2_2+1
        sty     idx2_3+1
        sty     idx2_4+1
        sty     idx2_5+1
        sty     idx2_6+1

        sty     idx3_1+1
        sty     idx3_2+1
        sty     idx3_3+1
        sty     idx3_4+1
        sty     idx3_5+1
        sty     idx3_6+1

        sty     idx4_1+1
        sty     idx4_2+1
        sty     idx4_4+1
        sty     idx4_5+1

        sty     idx5_1+1
        sty     idx5_2+1
        sty     idx5_4+1
        sty     idx5_5+1

        sty     idx6_1+1
        sty     idx6_2+1
        sty     idx6_4+1
        sty     idx6_5+1

        sty     idx7_1+1
        sty     idx7_2+1
        sty     idx7_4+1
        sty     idx7_5+1
        rts
.endproc
