; Too bad there's no #pragma align in cc65
        .export _normal_bits, _superfine_bits
        .export _normal_shift, _superfine_shift
        .export _idx
        .export _numbits, _bitpos, _valneg, _bitval
        .export _shift_table, _bits_table, _SCAN
        .export _nbits_avail, _bitmask_h, _bitmask_l, _negate_h, _negate_l
        .export _mul362_m, _mul362_l
        .export _mul473_m, _mul473_l
        .export _mul277_m, _mul277_l
        .export _mul669_m, _mul669_l
        .export _raw_image, _cache, _cache_start
        .export _actual_width, _total_blocks
        .export _blocks_per_row, _blocks_per_band, _blocks_rem_in_row
        .export _histogram_low, _histogram_high
        .export _orig_y_table_l, _orig_y_table_h
        .export _orig_x_offset, _special_x_orig_offset


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

_superfine_bits:        .byte 10,10,10,9,9,9,8,8
                        .byte 8,8,7,7,7,8,8,8
                        .byte 7,7,7,7,6,5,6,6
                        .byte 7,7,7,7,7,7,6,6
                        .byte 6,6,5,4,4,4,5,6
                        .byte 6,6,6,6,6,6,5,4
                        .byte 3,3,3,5,6,6,5,4
                        .byte 3,3,3,3,4,3,3,3

_normal_shift:          .byte 2,2,2,2,2,2,2,2
                        .byte 2,2,2,3,2,2,2,2
                        .byte 2,3,3,3,2,3,7,7
                        .byte 3,3,3,2,2,3,3,3
                        .byte 7,7,6,5,5,5,6,7
                        .byte 3,3,3,7,7,7,6,5
                        .byte 5,5,5,6,7,7,6,6
                        .byte 5,5,5,5,6,5,5,5

_superfine_shift:       .byte 0,0,0,0,0,0,1,1
                        .byte 1,1,1,1,1,1,1,1
                        .byte 1,1,1,1,1,1,1,1
                        .byte 1,1,1,1,1,1,1,1
                        .byte 1,1,1,1,1,1,1,1
                        .byte 1,1,1,1,1,1,1,1
                        .byte 2,2,2,1,1,1,1,2
                        .byte 2,2,2,2,2,2,2,2
.assert <* = 0, error

_SCAN:                  .byte  0, 1, 8,16, 9, 2, 3,10
                        .byte 17,24,32,25,18,11, 4, 5
                        .byte 12,19,26,33,40,48,41,34
                        .byte 27,20,13, 6, 7,14,21,28
                        .byte 35,42,49,56,57,50,43,36
                        .byte 29,22,15,23,30,37,44,51
                        .byte 58,59,52,45,38,31,39,46
                        .byte 53,60,61,54,47,55,62,63

_bitmask_h:             .byte %00000000
                        .byte %00000000
                        .byte %00000000
                        .byte %00000000
                        .byte %00000000
                        .byte %00000000
                        .byte %00000000
                        .byte %00000000
                        .byte %00000001
                        .byte %00000010
                        .byte %00000100
                        .byte %00001000
                        .byte %00010000
                        .byte %00100000
                        .byte %01000000
                        .byte %10000000

_bitmask_l:             .byte %00000001
                        .byte %00000010
                        .byte %00000100
                        .byte %00001000
                        .byte %00010000
                        .byte %00100000
                        .byte %01000000
                        .byte %10000000
                        .byte %00000000
                        .byte %00000000
                        .byte %00000000
                        .byte %00000000
                        .byte %00000000
                        .byte %00000000
                        .byte %00000000
                        .byte %00000000

_negate_h:              .byte %11111111
                        .byte %11111111
                        .byte %11111111
                        .byte %11111111
                        .byte %11111111
                        .byte %11111111
                        .byte %11111111
                        .byte %11111111
                        .byte %11111111
                        .byte %11111110
                        .byte %11111100
                        .byte %11111000
                        .byte %11110000
                        .byte %11100000
                        .byte %11000000
                        .byte %10000000

_negate_l:              .byte %11111111
                        .byte %11111110
                        .byte %11111100
                        .byte %11111000
                        .byte %11110000
                        .byte %11100000
                        .byte %11000000
                        .byte %10000000
                        .byte %00000000
                        .byte %00000000
                        .byte %00000000
                        .byte %00000000
                        .byte %00000000
                        .byte %00000000
                        .byte %00000000
                        .byte %00000000

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
_orig_y_table_l:        .res BAND_HEIGHT
_orig_y_table_h:        .res BAND_HEIGHT

_idx:                   .res 2

_bits_table:            .res 2
_shift_table:           .res 2
_actual_width:          .res 2
_total_blocks:          .res 2
_blocks_per_band:       .res 2
_blocks_per_row:        .res 1
_blocks_rem_in_row:     .res 1

_numbits:               .res 1
_bitpos:                .res 1
_valneg:                .res 1
_bitval:                .res 2
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
