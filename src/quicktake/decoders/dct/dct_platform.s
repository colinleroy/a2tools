        .export _mul_362, _mul_473, _mul_277, _mul_669
        .export _get_coeffs, _get_bitval, _cache_read
        .export _advance_block
        .export _shift_table, _bits_table

        .import _mul362_h, _mul362_m, _mul362_l
        .import _mul473_h, _mul473_m, _mul473_l
        .import _mul277_h, _mul277_m, _mul277_l
        .import _mul669_h, _mul669_m, _mul669_l

        .import _cache, _coef
        .import _bitmask_h, _bitmask_l, _bitpos
        .import _negate_h, _negate_l
        .import _ob, _SCAN, _scan
        .import _nbits_avail, _numbits, _bitval, _valneg

        .import _blocks_per_row, _blocks_rem_in_row
        .import _idx, _actual_width

        .import _ifd, _cache_start
        .import _read, _cputsxy
        .import decsp4, pushax 
        .importzp _prev_ram_irq_vector, _zp6, _zp7, c_sp

cur_cache_ptr     = _prev_ram_irq_vector ; Cache pointer, 2-bytes

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

_reading_str: .byte          "Reading     ", $0D, $0A, $00
_decoding_str:.byte          "Decoding    ", $0D, $0A, $00

.proc fill_cache
        stx     _zp6
        sty     _zp7
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
        ldx     _zp6
        ldy     _zp7
        jmp     inc_cache_done
.endproc

.proc inc_cache
        lda     #7
        sta     _nbits_avail
        inc     _cache_read
        bne     inc_cache_done
        inc     _cache_read+1
        lda     _cache_read+1
        cmp     #>CACHE_END
        bne     inc_cache_done
        jmp     fill_cache
.endproc

; Much left to optimize there
; Split read to avoid lda/ora/sta *2
; use carry for valneg once the rest is assembly
; use ZP
.proc _get_bitval
        lda     #0                      ; Init values
        sta     _valneg
        sta     _bitval
        sta     _bitval+1
        ldx     _bitpos                 ; bitpos is X

        ldy     _numbits                ; Get all bits
next_bit:
        dec     _nbits_avail
        bmi     inc_cache
inc_cache_done:
        lda     #0
cache_read = *+1
        lsr     $FFFF
        bcc     :+
        lda     _bitval+1               ; Update value
        ora     _bitmask_h,x
        sta     _bitval+1
        lda     _bitval
        ora     _bitmask_l,x
        sta     _bitval
        lda     #1
:       sta     _valneg                 ; update negative marker
        inx                             ; update bitpos
        dey
        bne     next_bit
        rts
.endproc
inc_cache_done = _get_bitval::inc_cache_done
_cache_read = _get_bitval::cache_read

.proc _get_coeffs
        ldx     #0
next_coeff:
        stx     _scan
        ldy     _SCAN,x

bits_table = *+1
        lda     $FFFF,x                 ; get numbits
        beq     inc_scan                ; eq jmp here
load_coef:
        sta     _numbits
shift_table = *+1
        lda     $FFFF,x                 ; get initial bitpos
        sta     _bitpos
        clc                             ; get ob
        adc     _numbits
        sta     _ob

        jsr     _get_bitval

        ldx     _scan                   ; is coef negative?
        beq     store_coef
        lda     _valneg
        beq     store_coef

        ldy     _ob                     ; extend sign bit
        lda     _bitval+1
        ora     _negate_h,y
        sta     _bitval+1
        lda     _bitval
        ora     _negate_l,y
        sta     _bitval
store_coef:
        ; coef[r] = (int8)(bitval >> (DESCALE_FACTOR+2));
        ldy     #(DESCALE_FACTOR+2)
        lda     _bitval
:       cmp     #$80
        ror     _bitval+1
        ror
        dey
        bne     :-
        ldy     _SCAN,x
inc_scan:
        sta     _coef,y                 ; zero, easy way out
        inx
        cpx     #64
        bcc     next_coeff
        rts
.endproc
_bits_table = _get_coeffs::bits_table
_shift_table = _get_coeffs::shift_table

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
        adc     _idx
        sta     _idx
        lda     _idx+1
        adc     #0
        sta     _idx+1
        rts
inc_row:
        ldx     _actual_width+1
        lda     row_step_l,x
        clc
        adc     _idx
        sta     _idx
        lda     row_step_h,x
        adc     _idx+1
        sta     _idx+1
        lda     _blocks_per_row
        sta     _blocks_rem_in_row
        rts
.endproc
