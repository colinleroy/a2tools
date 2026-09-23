; Too bad there's no #pragma align in cc65
        .export _normal_bits, _superfine_bits
        .export _normal_shift, _superfine_shift
        .export _idx
        .export _numbits, _bitpos
        .export _SCAN, _coef, _scan, _ign_bits
        .export _nbits_avail, _bitmask, _negate
        .export _mul362_m, _mul362_l
        .export _mul473_m, _mul473_l
        .export _mul277_m, _mul277_l
        .export _mul669_m, _mul669_l
        .export _raw_image, _cache, _cache_start
        .export _actual_width, _total_blocks
        .export _blocks_per_row, _blocks_per_band, _blocks_rem_in_row
        .export _tmp0, _tmp1, _tmp2, _tmp3, _tmp4, _tmp5, _tmp6, _tmp7
        .export _tmp10, _tmp11, _tmp12, _tmp13
        .export _z5, _z10, _z11, _z12, _z13
        .export _row_out
        .export _histogram_low, _histogram_high
        .export _orig_y_table_l, _orig_y_table_h
        .export _orig_x_offset, _special_x_orig_offset

        .export xbck, ybck, abck

        .importzp _zp6, _zp7, _zp8

xbck = _zp6
ybck = _zp7
abck = _zp8

DESCALE_FACTOR = 1
IGNORE_BITS_0 = (DESCALE_FACTOR+2-0)
IGNORE_BITS_1 = (DESCALE_FACTOR+2-1)
IGNORE_BITS_2 = (DESCALE_FACTOR+2-2)
IGNORE_BITS_3 = (DESCALE_FACTOR+2-3)

        .segment "DATA"
.align 256
_normal_bits:           .byte 8,8,8,7,7,7,7,7
                        .byte 7,7,6,5,6,7,7,7
                        .byte 6,5,5,5,5,3,0,0
                        .byte 5,5,5,6,6,5,4,4
                        .byte 0,0,0,0,0,0,0,0
                        .byte 4,4,4,0,0,0,0,0
                        .byte 0,0,0,0,0,0,0,0
                        .byte 0,0,0,0,0,0,0,0

_normal_shift:          .byte IGNORE_BITS_2,IGNORE_BITS_2,IGNORE_BITS_2,IGNORE_BITS_2,IGNORE_BITS_2,IGNORE_BITS_2,IGNORE_BITS_2,IGNORE_BITS_2
                        .byte IGNORE_BITS_2,IGNORE_BITS_2,IGNORE_BITS_2,IGNORE_BITS_3,IGNORE_BITS_2,IGNORE_BITS_2,IGNORE_BITS_2,IGNORE_BITS_2
                        .byte IGNORE_BITS_2,IGNORE_BITS_3,IGNORE_BITS_3,IGNORE_BITS_3,IGNORE_BITS_2,IGNORE_BITS_3,IGNORE_BITS_0,IGNORE_BITS_0
                        .byte IGNORE_BITS_3,IGNORE_BITS_3,IGNORE_BITS_3,IGNORE_BITS_2,IGNORE_BITS_2,IGNORE_BITS_3,IGNORE_BITS_3,IGNORE_BITS_3
                        .byte IGNORE_BITS_0,IGNORE_BITS_0,IGNORE_BITS_0,IGNORE_BITS_0,IGNORE_BITS_0,IGNORE_BITS_0,IGNORE_BITS_0,IGNORE_BITS_0
                        .byte IGNORE_BITS_3,IGNORE_BITS_3,IGNORE_BITS_3,IGNORE_BITS_0,IGNORE_BITS_0,IGNORE_BITS_0,IGNORE_BITS_0,IGNORE_BITS_0
                        .byte IGNORE_BITS_0,IGNORE_BITS_0,IGNORE_BITS_0,IGNORE_BITS_0,IGNORE_BITS_0,IGNORE_BITS_0,IGNORE_BITS_0,IGNORE_BITS_0
                        .byte IGNORE_BITS_0,IGNORE_BITS_0,IGNORE_BITS_0,IGNORE_BITS_0,IGNORE_BITS_0,IGNORE_BITS_0,IGNORE_BITS_0,IGNORE_BITS_0

_superfine_bits:        .byte 10,10,10,9,9,9,8,8
                        .byte 8,8,7,7,7,8,8,8
                        .byte 7,7,7,7,6,5,6,6
                        .byte 7,7,7,7,7,7,6,6
                        .byte 6,6,5,4,4,4,5,6
                        .byte 6,6,6,6,6,6,5,4
                        .byte 3,3,3,5,6,6,5,4
                        .byte 3,3,3,3,4,3,3,3

_superfine_shift:       .byte IGNORE_BITS_0,IGNORE_BITS_0,IGNORE_BITS_0,IGNORE_BITS_0,IGNORE_BITS_0,IGNORE_BITS_0,IGNORE_BITS_1,IGNORE_BITS_1
                        .byte IGNORE_BITS_1,IGNORE_BITS_1,IGNORE_BITS_1,IGNORE_BITS_1,IGNORE_BITS_1,IGNORE_BITS_1,IGNORE_BITS_1,IGNORE_BITS_1
                        .byte IGNORE_BITS_1,IGNORE_BITS_1,IGNORE_BITS_1,IGNORE_BITS_1,IGNORE_BITS_1,IGNORE_BITS_1,IGNORE_BITS_1,IGNORE_BITS_1
                        .byte IGNORE_BITS_1,IGNORE_BITS_1,IGNORE_BITS_1,IGNORE_BITS_1,IGNORE_BITS_1,IGNORE_BITS_1,IGNORE_BITS_1,IGNORE_BITS_1
                        .byte IGNORE_BITS_1,IGNORE_BITS_1,IGNORE_BITS_1,IGNORE_BITS_1,IGNORE_BITS_1,IGNORE_BITS_1,IGNORE_BITS_1,IGNORE_BITS_1
                        .byte IGNORE_BITS_1,IGNORE_BITS_1,IGNORE_BITS_1,IGNORE_BITS_1,IGNORE_BITS_1,IGNORE_BITS_1,IGNORE_BITS_1,IGNORE_BITS_1
                        .byte IGNORE_BITS_2,IGNORE_BITS_2,IGNORE_BITS_2,IGNORE_BITS_1,IGNORE_BITS_1,IGNORE_BITS_1,IGNORE_BITS_1,IGNORE_BITS_2
                        .byte IGNORE_BITS_2,IGNORE_BITS_2,IGNORE_BITS_2,IGNORE_BITS_2,IGNORE_BITS_2,IGNORE_BITS_2,IGNORE_BITS_2,IGNORE_BITS_2

.assert <* = 0, error

_SCAN:                  .byte  0*2, 1*2, 8*2,16*2, 9*2, 2*2, 3*2,10*2
                        .byte 17*2,24*2,32*2,25*2,18*2,11*2, 4*2, 5*2
                        .byte 12*2,19*2,26*2,33*2,40*2,48*2,41*2,34*2
                        .byte 27*2,20*2,13*2, 6*2, 7*2,14*2,21*2,28*2
                        .byte 35*2,42*2,49*2,56*2,57*2,50*2,43*2,36*2
                        .byte 29*2,22*2,15*2,23*2,30*2,37*2,44*2,51*2
                        .byte 58*2,59*2,52*2,45*2,38*2,31*2,39*2,46*2
                        .byte 53*2,60*2,61*2,54*2,47*2,55*2,62*2,63*2

_bitmask:               .byte %00000001
                        .byte %00000010
                        .byte %00000100
                        .byte %00001000
                        .byte %00010000
                        .byte %00100000
                        .byte %01000000
                        .byte %10000000

_negate:                .byte %11111111
                        .byte %11111110
                        .byte %11111100
                        .byte %11111000
                        .byte %11110000
                        .byte %11100000
                        .byte %11000000
                        .byte %10000000

_cache_start:           .addr _cache

        .segment "BSS"

; raw_image has 512px wide lines to help with alignment
; only the first 320 of each contain image data
; We'll fill in the blanks with the rest of the BSS data
; far enough that the scaler won't overwrite it (it will
; overwrite 256*16 bytes)
.align 256
_raw_image:             .res (BAND_HEIGHT)*RAW_WIDTH
_cache:                 .res CACHE_SIZE

.assert <* = 0, error
_histogram_low:         .res 256
_histogram_high:        .res 256
_orig_x_offset:         .res 256
_special_x_orig_offset: .res 256
_coef:                  .res 128
_row_out:               .res 128

_orig_y_table_l:        .res BAND_HEIGHT
_orig_y_table_h:        .res BAND_HEIGHT

_tmp0:                  .res 1
_tmp1:                  .res 1
_tmp2:                  .res 1
_tmp3:                  .res 1
_tmp4:                  .res 1
_tmp5:                  .res 1
_tmp6:                  .res 1
_tmp7:                  .res 1
_tmp10:                 .res 1
_tmp11:                 .res 1
_tmp12:                 .res 1
_tmp13:                 .res 1
_z5:                    .res 1
_z10:                   .res 1
_z11:                   .res 1
_z12:                   .res 1
_z13:                   .res 1

_idx:                   .res 2

_actual_width:          .res 2
_total_blocks:          .res 2
_blocks_per_band:       .res 2
_blocks_per_row:        .res 1
_blocks_rem_in_row:     .res 1

_scan:                  .res 1
_ign_bits:              .res 1
_numbits:               .res 1
_bitpos:                .res 1
_nbits_avail:           .res 1

        .segment "DATA"

.align 256
.proc _mul362_l
  .repeat 256, I
    .byte (I*362) .BITAND $FF
  .endrepeat
.endproc
.proc _mul362_m
  .repeat 256, I
    .byte ((I*362) .SHR 8) .BITAND $FF
  .endrepeat
.endproc
; .proc _mul362_h
;   .repeat 256, I
;     .byte ((I*362) .SHR 16) .BITAND $FF
;   .endrepeat
; .endproc

.proc _mul473_l
  .repeat 256, I
    .byte (I*473) .BITAND $FF
  .endrepeat
.endproc
.proc _mul473_m
  .repeat 256, I
    .byte ((I*473) .SHR 8) .BITAND $FF
  .endrepeat
.endproc
; .proc _mul473_h
;   .repeat 256, I
;     .byte ((I*473) .SHR 16) .BITAND $FF
;   .endrepeat
; .endproc

        .segment "LC"

.proc _mul277_l
  .repeat 256, I
    .byte (I*277) .BITAND $FF
  .endrepeat
.endproc
.proc _mul277_m
  .repeat 256, I
    .byte ((I*277) .SHR 8) .BITAND $FF
  .endrepeat
.endproc
; .proc _mul277_h
;   .repeat 256, I
;     .byte ((I*277) .SHR 16) .BITAND $FF
;   .endrepeat
; .endproc

.proc _mul669_l
  .repeat 256, I
    .byte (I*669) .BITAND $FF
  .endrepeat
.endproc
.proc _mul669_m
  .repeat 256, I
    .byte ((I*669) .SHR 8) .BITAND $FF
  .endrepeat
.endproc
; .proc _mul669_h
;   .repeat 256, I
;     .byte ((I*669) .SHR 16) .BITAND $FF
;   .endrepeat
; .endproc

.assert <* = 0, error
.proc right_shift_4
  .repeat 256, I
    .byte I .SHR 4
  .endrepeat
.endproc
