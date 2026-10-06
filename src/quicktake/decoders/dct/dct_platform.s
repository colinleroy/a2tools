        .export _get_coeffs, _cache_read
        .export _advance_block
        .export _idct_1d_rows, _idct_1d_cols
        .export _shift_table, _bits_table
        .export _init_idx, _update_idx
        .export _setup_floppy_restart
        .import floppy_motor_on

        .import _mul362_h, _mul362_m, _mul362_l
        .import _mul473_h, _mul473_m, _mul473_l
        .import _mul277_h, _mul277_m, _mul277_l
        .import _mul669_h, _mul669_m, _mul669_l
        .import block_step, row_step_l, row_step_h

        .import _asr1

        .import _coef, _row_out, _raw_image

        .import _cache
        .import _SCAN

        .import _blocks_per_row, _blocks_rem_in_row
        .import _image_size

        .import _ifd, _cache_start
        .import _read, _cputsxy
        .import decsp4, pushax

        .importzp _prev_ram_irq_vector, c_sp, tmp1
        .importzp xbck, ybck, _nbits_avail, _scan
        .importzp _tmp0, _tmp1, _tmp2, _tmp3, _tmp4, _tmp5, _tmp6, _tmp7
        .importzp _tmp10, _tmp11, _tmp12, _tmp13
        .importzp _z5, _z10, _z11, _z12, _z13_0, _z13_1

; Not a convenience to change. We want 1 for 7-bits precision
DESCALE_FACTOR = 1

CACHE_END = _cache + CACHE_SIZE
.assert <CACHE_END = 0, error

        .segment "CODE"

.proc _setup_floppy_restart
        lda     floppy_motor_on         ; Patch motor_on if we use a floppy
        beq     :+
        sta     start_floppy_motor+1
        lda     #$C0                    ; Firmware access space
        sta     start_floppy_motor+2
:       rts
.endproc

; int8 * x => >> 8 => (int8)
.macro do_mul TABL, TABM;, TABH
.scope
        bmi     neg
        tay
        lda     TABM,y
        jmp     done

neg:    clc
        eor     #$FF
        adc     #1
        tay

        clc
        lda     TABL,y
        eor     #$FF
        adc     #1
        lda     TABM,y
        eor     #$FF
        adc     #0
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

        .segment "LC"

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

        ; Unless it's a normal picture, in which case we can read
        ; only the remainder of the data we want. we caould do the
        ; same for the last reads of the other sizes but it's not
        ; really worth it (>8kB last read for normal, >4kB for
        ; superfine)
        ldy     _image_size
        bne     :+
        lda     #<((24*600)-INITIAL_CACHE_READ+$200)
        ldx     #>((24*600)-INITIAL_CACHE_READ+$200)
:       jsr     _read

        ldx     #0
        lda     #7
        jsr     pushax
        lda     #<_decoding_str
        ldx     #>_decoding_str
        jsr     _cputsxy
        jmp     cache_ok
.endproc

        .segment "CODE"

.proc inc_cache
        ldx     #7
        inc     _cache_read
        bne     inc_cache_done
        inc     _cache_read+1
        bne     inc_cache_done          ; eq. JMP here, cache end is checked once per block
.endproc

; 64 coefficients - 48 bytes per coeff
; LSR buffer: 48 lsr abs + 48 ror A = 384 cycles per block (+2 lda abs)
; or load buffer and ROR ZP: 48 LSR A + 48 ror ZP = 336 cycles + 2 LDA imm, 3 STA ZP, (4 LDA ABS*48) = 528
; LSR abs+ror A wins

start_floppy_motor:
        sta     motor_on                ; Patched if on floppy
        cpx     #>CACHE_END             ; Check for cache end and refill cache
        bne     cache_ok
        jmp     fill_cache

.proc _get_coeffs
        ldx     _cache_read+1           ; Cache end ?
        cpx     #(>CACHE_END)-4
        bpl     start_floppy_motor
cache_ok:
        ldx     #8
        stx     _nbits_avail
        ldy     #0                      ; _scan iterator
next_coeff:
shift_table = *+1
        lda     $FFFF,y                 ; get ignored bits shift table
        beq     inc_scan                ; ignored bits 0 means bits = 0, skip all
        sta     asrtab+2                ; update shift/sign table to the correct one

bits_table = *+1
        lda     $FFFF,y                 ; get numbits
        sty     _scan
        tay                             ; num_bits in Y

        ; lda     #0                    ; No need to init bitval to 0 as we'll shift min 8 bits into it
        ldx     _nbits_avail

next_bit:
        dex
        bmi     inc_cache               ; No more bits in cur byte, increment cache pointer
inc_cache_done:
_cache_read = *+1
        lsr     $FFFF                   ; Get next bit
        ror
        dey
        bne     next_bit

        stx     _nbits_avail            ; Remember how many bits we have

        tay
asrtab:
        lda     $FF00,y                 ; Shift and sign-extend
        ldy     _scan                   ; Reload _scan for caller
        beq     force_pos               ; if scan == 0, force coef[0] positive
got_bits:

        ldx     _SCAN,y                 ; Load coef number
        sta     _coef,x                 ; And store it
inc_scan:
        iny
        cpy     #64
        bcc     next_coeff

        inc     _cache_read             ; Increment cache pointer, all bits consumed
        bne     :+
        inc     _cache_read+1
:       rts

force_pos:
        and     #$7F
        jmp     got_bits
.endproc
_bits_table    = _get_coeffs::bits_table
_shift_table   = _get_coeffs::shift_table
inc_cache_done = _get_coeffs::inc_cache_done
cache_ok       = _get_coeffs::cache_ok
_cache_read    = _get_coeffs::_cache_read

.proc inc_row
        ldx     _image_size
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

.proc _advance_block
        dec     _blocks_rem_in_row
        beq     inc_row
inc_block:
        ldx     _image_size
        lda     block_step,x
        clc
        adc     idx0_1+1
        sta     idx0_1+1
        bcc     _update_idx
        inc     idx0_1+2
        ; fallthrough
.endproc
.proc _update_idx
        ldy     _image_size
        beq     patch_small
patch_large:
        .assert <_raw_image = 0, error
        ldy     idx0_1+1
        sty     idx0_3+1
        sty     idx0_6+1

        sty     idx1_3+1
        sty     idx1_6+1

        sty     idx2_3+1
        sty     idx2_6+1

        sty     idx3_3+1
        sty     idx3_6+1

        ldy     idx0_1+2
prev_idx0l = *+1
        cpy     #$00                    ; Don't patch unchanged high bytes
        bne     :+
        rts
:       sty     prev_idx0l
        sty     idx0_3+2
        sty     idx0_6+2
        .assert RAW_WIDTH = 512, error
        iny
        iny
        sty     idx1_3+2
        sty     idx1_6+2

        iny
        iny
        sty     idx2_3+2
        sty     idx2_6+2

        iny
        iny
        sty     idx3_3+2
        sty     idx3_6+2
        rts

patch_small:
        .assert <_raw_image = 0, error
        ldy     idx0_1+1
        sty     idx0_2+1
        sty     idx0_4+1
        sty     idx0_5+1

        sty     idx1_1+1
        sty     idx1_2+1
        sty     idx1_4+1
        sty     idx1_5+1

        sty     idx2_1+1
        sty     idx2_2+1
        sty     idx2_4+1
        sty     idx2_5+1

        sty     idx3_1+1
        sty     idx3_2+1
        sty     idx3_4+1
        sty     idx3_5+1

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
prev_idx0s = *+1
        cpy     #$00                    ; Don't patch unchanged high bytes
        bne     :+
        rts
:       sty     prev_idx0s
        sty     idx0_2+2
        sty     idx0_4+2
        sty     idx0_5+2
        .assert RAW_WIDTH = 512, error
        iny
        iny
        sty     idx1_1+2
        sty     idx1_2+2
        sty     idx1_4+2
        sty     idx1_5+2

        iny
        iny
        sty     idx2_1+2
        sty     idx2_2+2
        sty     idx2_4+2
        sty     idx2_5+2

        iny
        iny
        sty     idx3_1+2
        sty     idx3_2+2
        sty     idx3_4+2
        sty     idx3_5+2

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
next_x:
        tax
        lda     _coef+2,x
        ora     _coef+4,x
        ora     _coef+6,x
        ora     _coef+8,x
        ora     _coef+10,x
        ora     _coef+12,x
        ora     _coef+14,x
        bne     full_rows

        lda     _coef+0,x               ; Easy way, all AC = 0
        sta     _row_out+0,x
        sta     _row_out+2,x
        sta     _row_out+4,x
        sta     _row_out+6,x
        sta     _row_out+8,x
        sta     _row_out+10,x
        sta     _row_out+12,x
        sta     _row_out+14,x

        txa
        clc
        adc     #16
        bmi     :+                      ; > 128
        jmp     next_x
:       rts

full_rows:
        ; tmp13 = CLAMPI(coef[y + 4] + coef[y + 12]);
        lda     _coef+4,x
        ADDIX   _coef+12
        sta     _tmp13

        ; tmp10 = CLAMPI(coef[y + 0] + coef[y + 8]);
        lda     _coef+0,x
        ADDIX   _coef+8
        sta     _tmp10

        ; tmp0 = CLAMPI(tmp10 + tmp13);
        ADDI    _tmp13
        sta     _tmp0

        ; tmp3 = CLAMPI(tmp10 - tmp13);
        lda     _tmp10
        SUBI    _tmp13
        sta     _tmp3

        ; tmp11 = CLAMPI(coef[y + 0] - coef[y + 8]);
        lda     _coef+0,x
        SUBIX   _coef+8
        sta     _tmp11

        ; tmp12 = CLAMPI(mul_362(CLAMPI(coef[y + 4] - coef[y + 12])) - tmp13);
        lda     _coef+4,x
        SUBIX   _coef+12
        MULT_362
        SUBI    _tmp13
        sta     _tmp12

        ; tmp1 = CLAMPI(tmp11 + tmp12);
        ADDI    _tmp11
        sta     _tmp1

        ; tmp2 = CLAMPI(tmp11 - tmp12);
        lda     _tmp11
        SUBI    _tmp12
        sta     _tmp2

        ; z10   = CLAMPI(coef[y + 10] - coef[y + 6]);
        lda     _coef+10,x
        SUBIX   _coef+6
        sta     _z10

        ; z13_1 = mul_669(z10);
        MULT_669
        sta     _z13_1

        ; z11   = CLAMPI(coef[y + 2] + coef[y + 14]);
        lda     _coef+2,x
        ADDIX   _coef+14
        sta     _z11

        ; z13_0 = CLAMPI(coef[y + 10] + coef[y + 6]);
        lda     _coef+10,x
        ADDIX   _coef+6
        sta     _z13_0

        ; tmp7 = CLAMPI(z11 + z13_0);
        ADDI    _z11
        sta     _tmp7

        ; row_out[y + 0]  = CLAMPI(tmp0 + tmp7);
        lda     _tmp0
        ADDI    _tmp7
        sta     _row_out+0,x

        ; z12   = CLAMPI(coef[y + 2] - coef[y + 14]);
        lda     _coef+2,x
        SUBIX   _coef+14
        sta     _z12

        ; z5 = (mul_473(CLAMPI(z10 + z12)));
        ADDI    _z10
        MULT_473
        sta     _z5

        ; tmp6 = CLAMPI(CLAMPI(z5 - z13_1) - tmp7);
        SUBI    _z13_1
        SUBI    _tmp7
        sta     _tmp6

        ; row_out[y + 2]  = CLAMPI(tmp1 + tmp6);
        ADDI    _tmp1
        sta     _row_out+2,x

        ; tmp10 = CLAMPI(mul_277(z12) - z5);
        lda     _z12
        MULT_277
        SUBI    _z5
        sta     _tmp10

        ; tmp5 = CLAMPI(mul_362(CLAMPI(z11 - z13_0)) - tmp6);
        lda     _z11
        SUBI    _z13_0
        MULT_362
        SUBI    _tmp6
        sta     _tmp5

        ADDI    _tmp10
        sta     _tmp4

        ADDI    _tmp3
        sta     _row_out+8,x

        lda     _tmp2
        ADDI    _tmp5
        sta     _row_out+4,x

        lda     _tmp3
        SUBI    _tmp4
        sta     _row_out+6,x

        lda     _tmp2
        SUBI    _tmp5
        sta     _row_out+10,x

        lda     _tmp1
        SUBI    _tmp6
        sta     _row_out+12,x

        lda     _tmp0
        SUBI    _tmp7
        sta     _row_out+14,x

        txa
        clc
        adc     #16
        bmi     :+                      ; > 128
        jmp     next_x
:       rts
.endproc

; uint8 clamp of int8 x << 1: (sign bit goes to carry)
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

        lda     _image_size
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
        ldy     _asr1,x                 ; Y = X/2 (X < 128)

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
        ; tmp13   = CLAMPI(row_out[x + 32] + row_out[x + 96]);
        lda     _row_out+32,x
        ADDIX   _row_out+96
        sta     _tmp13

        ; tmp10 = CLAMPI(row_out[x + 0]  + row_out[x + 64]);
        lda     _row_out+0,x
        ADDIX   _row_out+64
        sta     _tmp10

        ; tmp0 = CLAMPI(tmp10 + tmp13);
        ADDI    _tmp13
        sta     _tmp0

        ; tmp3 = CLAMPI(tmp10 - tmp13);
        lda     _tmp10
        SUBI    _tmp13
        sta     _tmp3

        ; tmp11 = CLAMPI(row_out[x + 0]  - row_out[x + 64]);
        lda     _row_out+0,x
        SUBIX   _row_out+64
        sta     _tmp11

        ; tmp12 = CLAMPI(mul_362(CLAMPI(row_out[x + 32] - row_out[x + 96])) - tmp13);
        lda     _row_out+32,x
        SUBIX   _row_out+96
        MULT_362
        SUBI    _tmp13
        sta     _tmp12

        ; tmp1 = CLAMPI(tmp11 + tmp12);
        ADDI    _tmp11
        sta     _tmp1

        ; tmp2 = CLAMPI(tmp11 - tmp12);
        lda     _tmp11
        SUBI    _tmp12
        sta     _tmp2

        ; z10     = CLAMPI(row_out[x + 80] - row_out[x + 48]);
        lda     _row_out+80,x
        SUBIX   _row_out+48
        sta     _z10

        ; z13_1 = mul_669(z10);
        lda     _z10
        MULT_669
        sta     _z13_1

        ; z11     = CLAMPI(row_out[x + 16] + row_out[x + 112]);
        lda     _row_out+16,x
        ADDIX   _row_out+112
        sta     _z11

        ; z13_0   = CLAMPI(row_out[x + 80] + row_out[x + 48]);
        lda     _row_out+80,x
        ADDIX   _row_out+48
        sta     _z13_0

        ; tmp7 = CLAMPI(z11 + z13_0);
        ADDI    _z11
        sta     _tmp7

        ; z12     = CLAMPI(row_out[x + 16] - row_out[x + 112]);
        lda     _row_out+16,x
        SUBIX   _row_out+112
        sta     _z12

        ; z5 = mul_473(CLAMPI(z10 + z12));
        ADDI    _z10
        MULT_473
        sta     _z5

        ; tmp6 = CLAMPI(CLAMPI(z5 - z13_1) - tmp7);
        SUBI    _z13_1
        SUBI    _tmp7
        sta     _tmp6

        ; tmp10 = CLAMPI(mul_277(z12) - z5);
        lda     _z12
        MULT_277
        SUBI    _z5
        sta     _tmp10

        ; tmp5 = CLAMPI(mul_362(CLAMPI(z11 - z13_0)) - tmp6);
        lda     _z11
        SUBI    _z13_0
        MULT_362
        SUBI    _tmp6
        sta     _tmp5

        ; tmp4 = CLAMPI(tmp5 + tmp10);
        ADDI    _tmp10
        sta     _tmp4
; idct_common end
        ldy     _image_size
        beq     full_cols_no_scale
        jmp     full_cols_scale_down

full_cols_no_scale:
        txa
        tay
        iny                             ; Y = X+1

        lda     _tmp0
        ADDU    _tmp7
idx0_4: sta     $FFFF,x
idx0_5: sta     $FFFF,y

        lda     _tmp2
        ADDU    _tmp5
idx2_4: sta     $FFFF,x
idx2_5: sta     $FFFF,y

        lda     _tmp4
        ADDU    _tmp3
idx4_4: sta     $FFFF,x
idx4_5: sta     $FFFF,y

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
        ldy     _asr1,x

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

.segment "BSS"
motor_on: .res 2
