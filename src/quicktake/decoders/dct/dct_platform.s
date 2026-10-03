        .export _get_coeffs, _cache_read
        .export _advance_block
        .export _idct_1d_rows, _idct_1d_cols
        .export _shift_table, _bits_table
        .export _init_idx, _update_idx

        .import _mul362_h, _mul362_m, _mul362_l
        .import _mul473_h, _mul473_m, _mul473_l
        .import _mul277_h, _mul277_m, _mul277_l
        .import _mul669_h, _mul669_m, _mul669_l

        .import _lsr_tables, _asr_tables
        
        .import _coef, _coef_sign, _row_out, _raw_image

        .import _cache
        .import _SCAN

        .import _blocks_per_row, _blocks_rem_in_row
        .import _actual_width

        .import _ifd, _cache_start
        .import _read, _cputsxy
        .import decsp4, pushax

        .importzp _prev_ram_irq_vector, c_sp, tmp1
        .importzp xbck, ybck, _nbits_avail, _scan, _ign_bits
        .importzp _tmp0, _tmp1, _tmp2, _tmp3, _tmp4, _tmp5, _tmp6, _tmp7
        .importzp _tmp10, _tmp11, _tmp12, _tmp13
        .importzp _z5, _z10, _z11, _z12, _z13_0, _z13_1

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
        rts
.endproc

.proc inc_cache
        ldx     #7
        inc     _cache_read
        bne     inc_cache_done
        inc     _cache_read+1
        bne     inc_cache_done
.endproc

; 64 coefficients - 48 bytes per coeff
; LSR buffer: 48 lsr abs + 48 ror A = 384 cycles per block (+2 lda abs)
; or load buffer and ROR ZP: 48 LSR A + 48 ror ZP = 336 cycles + 2 LDA imm, 3 STA ZP, (4 LDA ABS*48) = 528
; LSR abs+ror A wins

; Enter with numbits in Y, _ign_bits in X
; Exits with carry set if last bit set (neg)
; Exits with bitval in A
.macro GET_BITVAL
        lda     #0                      ; Init bitval
        ldx     _nbits_avail
next_bit:
        dex
        bmi     inc_cache
inc_cache_done:
_cache_read = *+1
        lsr     $FFFF
        ror
        dey
        bne     next_bit

        stx     _nbits_avail            ; Remember how many bits we have

        ldx     _ign_bits               ; Preload bits to shift out
        cmp     #$80                    ; is last bit 1 (negative) ?
        bcc     shift_pos
shift_neg:
        ldy     _scan                   ; is scan != 0? (note: caller expects _scan in Y)
        beq     shift_pos               ; if scan == 0 ignore sign extension
        ldy     _asr_tables,x
        sty     asrtab+2
        tax
asrtab:
        lda     $FF00,x
;         ldy     _ign_bits               ; Need a copy for iterating
; :       lsr
;         dey
;         bne     :-
;         ora     _coef_sign,x
        jmp     done
shift_pos:
        ldy     _lsr_tables,x
        sty     lsrtab+2
        tax
lsrtab:
        lda     $FF00,x
; :       lsr
;         dex
;         bne     :-
done:
        ldy     _scan
.endmacro

.proc _get_coeffs
        ldx     _cache_read+1           ; Cache end ?
        cpx     #>CACHE_END
        bne     :+
        jsr     fill_cache
:       ldx     #8
        stx     _nbits_avail
        ldy     #0
next_coeff:
shift_table = *+1
        lda     $FFFF,y                 ; get ignored bits shift
        beq     inc_scan                ; eq jmp here, no bits shift = no bits = coef 0
        sta     _ign_bits

bits_table = *+1
        lda     $FFFF,y                 ; get numbits
        sty     _scan
        tay                             ; num_bits in Y

        GET_BITVAL
        ldx     _SCAN,y
        sta     _coef,x
inc_scan:
        iny
        cpy     #64
        bcc     next_coeff

        inc     _cache_read             ; Increment cache pointer
        bne     :+
        inc     _cache_read+1
:       rts
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

; int8 clamp of sum/sub:
; x = (a + b) or (a - b)
; no overflow               ? good
; overflow and result > 127 ? 127
; overflow and result < 127 ? -128
.macro CLAMPI b
        bvc     done
        bmi     plus
        lda     #$80
        bne     done
plus:   lda     #$7F
done:
.endmacro

.macro ADDI b
.scope
        clc
        adc     b
        CLAMPI
.endscope
.endmacro
.macro SUBI b
.scope
        sec
        sbc     b
        CLAMPI
.endscope
.endmacro

.macro ADDIY b
.scope
        clc
        adc     b,y
        CLAMPI
.endscope
.endmacro
.macro SUBIY b
.scope
        sec
        sbc     b,y
        CLAMPI
.endscope
.endmacro

.macro ADDIX b
.scope
        clc
        adc     b,x
        CLAMPI
.endscope
.endmacro
.macro SUBIX b
.scope
        sec
        sbc     b,x
        CLAMPI
.endscope
.endmacro

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
        ADDIY   _coef+8
        sta     _tmp10
        lda     _coef+0,y
        SUBIY   _coef+8
        sta     _tmp11

        lda     _coef+4,y
        SUBIY   _coef+12
        sta     _tmp12

        lda     _coef+4,y
        ADDIY   _coef+12
        sta     _tmp13

        ADDI    _tmp10                  ; tmp0 = CLAMPI(tmp10 + tmp13);
        sta     _tmp0
        lda     _tmp10                  ; tmp3 = CLAMPI(tmp10 - tmp13);
        SUBI    _tmp13
        sta     _tmp3

        lda     _coef+10,y
        SUBIY   _coef+6
        sta     _z10

        lda     _coef+2,y
        ADDIY   _coef+14
        sta     _z11

        lda     _coef+10,y
        ADDIY   _coef+6
        sta     _z13_0

        ADDI    _z11
        sta     _tmp7

        lda     _coef+2,y
        SUBIY   _coef+14
        sta     _z12

        sty     ybck                    ; Backup Y before mults
; idct_common start

        ADDI    _z10
        MULT_473
        sta     _z5

        lda     _tmp12
        MULT_362
        SUBI    _tmp13
        sta     _tmp12

        ADDI    _tmp11
        sta     _tmp1

        lda     _tmp11
        SUBI    _tmp12
        sta     _tmp2

        lda     _z10
        MULT_669
        sta     _z13_1

        lda     _z5
        SUBI    _z13_1
        SUBI    _tmp7
        sta     _tmp6

        lda     _z12
        MULT_277
        SUBI    _z5
        sta     _tmp10

        lda     _z11
        SUBI    _z13_0
        MULT_362
        SUBI    _tmp6
        sta     _tmp5

        ADDI    _tmp10
        sta     _tmp4
; idct_common end
        ldy     ybck

        lda     _tmp0
        ADDI    _tmp7
        sta     _row_out+0,y

        lda     _tmp1
        ADDI    _tmp6
        sta     _row_out+2,y

        lda     _tmp2
        ADDI    _tmp5
        sta     _row_out+4,y

        lda     _tmp3
        SUBI    _tmp4
        sta     _row_out+6,y

        lda     _tmp3
        ADDI    _tmp4
        sta     _row_out+8,y

        lda     _tmp2
        SUBI    _tmp5
        sta     _row_out+10,y

        lda     _tmp1
        SUBI    _tmp6
        sta     _row_out+12,y

        lda     _tmp0
        SUBI    _tmp7
        sta     _row_out+14,y

        tya
        clc
        adc     #16
        bmi     :+                      ; > 128
        jmp     next_y
:       rts
.endproc

; uint8 clamp of int8 x << 1:
; x < 0  ? => 0
; x >= 0 ? => x<<1
.macro CLAMPU
.scope
        asl
        bcc     :+
        lda     #0
:
.endscope
.endmacro

; Organized in a not-extremely obvious way, to minimize
; cost on standard path (no overflow).
; depending on (a + b) or (a - b)
; x < 0    ?  => 0
; x 0..127 ?  => 0..254
; x > 127  ?  => 255
.macro ADD_CLAMPU
.scope
        bvc     inrange
        bpl     oneg
opos:   lda     #$FF
        bne     done
oneg:   lda     #$00
        beq     done
inrange:
        bmi     oneg
        asl
done:
.endscope
.endmacro

.macro ADDU b
.scope
        clc
        adc     b
        ADD_CLAMPU
.endscope
.endmacro

.macro SUBU b
.scope
        sec
        sbc     b
        ADD_CLAMPU
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

        lda     _row_out+32,x
        ADDIX   _row_out+96
        sta     _tmp13

        lda     _row_out+80,x
        SUBIX   _row_out+48
        sta     _z10

        lda     _row_out+16,x
        ADDIX   _row_out+112
        sta     _z11

        lda     _row_out+16,x
        SUBIX   _row_out+112
        sta     _z12

        lda     _row_out+80,x
        ADDIX   _row_out+48
        sta     _z13_0

        ADDI    _z11
        sta     _tmp7

        lda     _row_out+0,x
        ADDIX   _row_out+64
        sta     _tmp10

        ADDI    _tmp13
        sta     _tmp0

        lda     _tmp10
        SUBI    _tmp13
        sta     _tmp3

        lda     _row_out+0,x
        SUBIX   _row_out+64
        sta     _tmp11

        lda     _row_out+32,x
        SUBIX   _row_out+96

        stx     xbck                    ; Keep X before mults

        MULT_362
        SUBI    _tmp13
        sta     _tmp12

        ADDI    _tmp11
        sta     _tmp1

        lda     _tmp11
        SUBI    _tmp12
        sta     _tmp2

        lda     _z10
        MULT_669
        sta     _z13_1

        lda     _z10
        ADDI    _z12
        MULT_473
        sta     _z5

        SUBI    _z13_1
        SUBI    _tmp7
        sta     _tmp6

        lda     _z12
        MULT_277
        SUBI    _z5
        sta     _tmp10

        lda     _z11
        SUBI    _z13_0
        MULT_362
        SUBI    _tmp6
        sta     _tmp5

        ADDI    _tmp10
        sta     _tmp4
; idct_common end
        ldx     xbck

        ldy     _actual_width
        cpy     #<160
        beq     full_cols_no_scale
        jmp     full_cols_scale_down

full_cols_no_scale:
        txa
        tay
        iny                             ; Y = X+1

        ADDU    _tmp3
idx4_4: sta     $FFFF,x
idx4_5: sta     $FFFF,y

        lda     _tmp0
        ADDU    _tmp7
idx0_4: sta     $FFFF,x
idx0_5: sta     $FFFF,y

        lda     _tmp2
        ADDU    _tmp5
idx2_4: sta     $FFFF,x
idx2_5: sta     $FFFF,y

        lda     _tmp1
        SUBU    _tmp6
idx6_4: sta     $FFFF,x
idx6_5: sta     $FFFF,y

        lda     _tmp1
        ADDU    _tmp6
idx1_4: sta     $FFFF,x
idx1_5: sta     $FFFF,y

        lda     _tmp3
        SUBU    _tmp4
idx3_4: sta     $FFFF,x
idx3_5: sta     $FFFF,y

        lda     _tmp2
        SUBU    _tmp5
idx5_4: sta     $FFFF,x
idx5_5: sta     $FFFF,y

        lda     _tmp0
        SUBU    _tmp7
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
        ADDU    _tmp7
idx0_6: sta     $FFFF,y

        lda     _tmp2
        ADDU    _tmp5
idx1_6: sta     $FFFF,y

        lda     _tmp3
        ADDU    _tmp4
idx2_6: sta     $FFFF,y

        lda     _tmp1
        SUBU    _tmp6
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

        ldy     idx0_1+2
prev_idx0 = *+1
        cpy     #$00                    ; Don't patch unchanged high bytes
        bne     :+
        rts
:       sty     prev_idx0
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
        rts
.endproc
