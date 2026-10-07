; Too bad there's no #pragma align in cc65
        .export _normal_bits, _superfine_bits
        .export _normal_shift, _superfine_shift
        .export _lsr1
        .export _idx
        .export _SCAN, _coef, _scan
        .export _mul362_m, _mul362_l
        .export _mul473_m, _mul473_l
        .export _mul277_m, _mul277_l
        .export _mul669_m, _mul669_l
        .export _raw_image, _cache, _cache_start
        .export _image_size
        .export _blocks_per_row, _blocks_per_band, _blocks_rem_in_row
        .export _tmp0, _tmp1, _tmp2, _tmp3, _tmp4, _tmp5, _tmp6, _tmp7
        .export _tmp10, _tmp11, _tmp12, _tmp13
        .export _z5, _z10, _z11, _z12, _z13_0, _z13_1
        .export block_step, row_step_l, row_step_h
        .export _row_out
        .export _histogram_low, _histogram_high
        .export _orig_y_table_l, _orig_y_table_h
        .export _orig_x_offset, _special_x_orig_offset

        .importzp _zp6, _zp7, _zp8, _zp9, _zp10, _zp11, _zp12, _zp13
        .importzp tmp3, tmp4, ptr1, ptr2, ptr3, ptr4

; For _get_coeffs
_scan        = _zp8
_ign_bits    = _zp10

; For idct
_tmp0       = _zp6
_tmp1       = _zp7
_tmp2       = _zp8
_tmp3       = _zp9
_tmp4       = _zp10
_tmp5       = _zp11
_tmp6       = _zp12
_tmp7       = _zp13
_tmp10      = ptr1
_tmp11      = ptr1+1
_tmp12      = ptr2
_tmp13      = ptr2+1
_z5         = ptr3
_z10        = ptr3+1
_z11        = ptr4
_z12        = ptr4+1
_z13_0      = tmp3
_z13_1      = tmp4

        .segment "DATA"
.align 256
.assert <* = 0, error

.proc _lsr1
                        .repeat 256, I
                          .byte I .SHR 1
                        .endrepeat
.endproc

.proc _asr1
                        .repeat 128, I
                          .byte I .SHR 1
                        .endrepeat
                        .repeat 128, I
                          .byte ((I+128) .SHR 1) .BITOR $80
                        .endrepeat
.endproc

.proc _asr2
                        .repeat 128, I
                          .byte I .SHR 2
                        .endrepeat
                        .repeat 128, I
                          .byte ((I+128) .SHR 2) .BITOR $C0
                        .endrepeat
.endproc

.proc _asr3
                        .repeat 128, I
                          .byte I .SHR 3
                        .endrepeat
                        .repeat 128, I
                          .byte ((I+128) .SHR 3) .BITOR $E0
                        .endrepeat
.endproc

.proc _asr4
                        .repeat 128, I
                          .byte I .SHR 4
                        .endrepeat
                        .repeat 128, I
                          .byte ((I+128) .SHR 4) .BITOR $F0
                        .endrepeat
.endproc

.proc _asr5
                        .repeat 128, I
                          .byte I .SHR 5
                        .endrepeat
                        .repeat 128, I
                          .byte ((I+128) .SHR 5) .BITOR $F8
                        .endrepeat
.endproc

.proc _asr6
                        .repeat 128, I
                          .byte I .SHR 6
                        .endrepeat
                        .repeat 128, I
                          .byte ((I+128) .SHR 6) .BITOR $FC
                        .endrepeat
.endproc

.assert <* = 0, error

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

.proc _mul473_l
                        .repeat 256, I
                          .byte (I*473) .BITAND $FF
                        .endrepeat
.endproc
; continued in LC below

.assert <* = 0, error

; low byte of coef[0]
COEF_LOW = <(_raw_image+DECODE_WIDTH+(BAND_HEIGHT-4)*RAW_WIDTH)
; Where to store coefficients, including low byte offset in page, for performance in _get_coeffs
.proc _SCAN
                        .byte  0*2+COEF_LOW, 1*2+COEF_LOW, 8*2+COEF_LOW,16*2+COEF_LOW, 9*2+COEF_LOW, 2*2+COEF_LOW, 3*2+COEF_LOW,10*2+COEF_LOW
                        .byte 17*2+COEF_LOW,24*2+COEF_LOW,32*2+COEF_LOW,25*2+COEF_LOW,18*2+COEF_LOW,11*2+COEF_LOW, 4*2+COEF_LOW, 5*2+COEF_LOW
                        .byte 12*2+COEF_LOW,19*2+COEF_LOW,26*2+COEF_LOW,33*2+COEF_LOW,40*2+COEF_LOW,48*2+COEF_LOW,41*2+COEF_LOW,34*2+COEF_LOW
                        .byte 27*2+COEF_LOW,20*2+COEF_LOW,13*2+COEF_LOW, 6*2+COEF_LOW, 7*2+COEF_LOW,14*2+COEF_LOW,21*2+COEF_LOW,28*2+COEF_LOW
                        .byte 35*2+COEF_LOW,42*2+COEF_LOW,49*2+COEF_LOW,56*2+COEF_LOW,57*2+COEF_LOW,50*2+COEF_LOW,43*2+COEF_LOW,36*2+COEF_LOW
                        .byte 29*2+COEF_LOW,22*2+COEF_LOW,15*2+COEF_LOW,23*2+COEF_LOW,30*2+COEF_LOW,37*2+COEF_LOW,44*2+COEF_LOW,51*2+COEF_LOW
                        .byte 58*2+COEF_LOW,59*2+COEF_LOW,52*2+COEF_LOW,45*2+COEF_LOW,38*2+COEF_LOW,31*2+COEF_LOW,39*2+COEF_LOW,46*2+COEF_LOW
                        .byte 53*2+COEF_LOW,60*2+COEF_LOW,61*2+COEF_LOW,54*2+COEF_LOW,47*2+COEF_LOW,55*2+COEF_LOW,62*2+COEF_LOW,63*2+COEF_LOW
.endproc

; $00 = nobits
; coef[0] has lsr instead of asr as it's always positive
.proc _normal_shift
                        .byte >_lsr1, >_asr1, >_asr1, >_asr2, >_asr2, >_asr2, >_asr2, >_asr2
                        .byte >_asr2, >_asr2, >_asr3, >_asr3, >_asr3, >_asr2, >_asr2, >_asr2
                        .byte >_asr3, >_asr3, >_asr3, >_asr3, >_asr4, >_asr5, $00, $00
                        .byte >_asr3, >_asr3, >_asr3, >_asr3, >_asr3, >_asr3, >_asr4, >_asr4
                        .byte $00, $00, $00, $00, $00, $00, $00, $00
                        .byte >_asr4, >_asr4, >_asr4, $00, $00, $00, $00, $00
                        .byte $00, $00, $00, $00, $00, $00, $00, $00
                        .byte $00, $00, $00, $00, $00, $00, $00, $00
.endproc

.proc _superfine_shift
                        .byte >_lsr1, >_asr1, >_asr1, >_asr2, >_asr2, >_asr2, >_asr2, >_asr2
                        .byte >_asr2, >_asr2, >_asr3, >_asr3, >_asr3, >_asr2, >_asr2, >_asr2
                        .byte >_asr3, >_asr3, >_asr3, >_asr3, >_asr4, >_asr5, >_asr4, >_asr4
                        .byte >_asr3, >_asr3, >_asr3, >_asr3, >_asr3, >_asr3, >_asr4, >_asr4
                        .byte >_asr4, >_asr4, >_asr5, >_asr6, >_asr6, >_asr6, >_asr5, >_asr4
                        .byte >_asr4, >_asr4, >_asr4, >_asr4, >_asr4, >_asr4, >_asr5, >_asr6
                        .byte >_asr6, >_asr6, >_asr6, >_asr5, >_asr4, >_asr4, >_asr5, >_asr5
                        .byte >_asr6, >_asr6, >_asr6, >_asr6, >_asr5, >_asr6, >_asr6, >_asr6
.endproc

; Default for fine/superfine images
_image_size:            .byte 1
_blocks_per_band:       .word 2400/(DECODE_HEIGHT/BAND_HEIGHT)
_blocks_per_row:        .byte 40

_cache_start:           .addr _cache

        .segment "BSS"

; raw_image has 512px wide lines to help with alignment
; only the first 320 of each contain image data
; We'll fill in the blanks with the rest of the BSS data
; far enough that the scaler won't overwrite it (it will
; overwrite 256*16 bytes)
.align 256
_cache:                 .res CACHE_SIZE
_histogram_low:         .res 256
_histogram_high:        .res 256
_orig_x_offset:         .res 256
_special_x_orig_offset: .res 256
_raw_image:             .res (BAND_HEIGHT-1)*RAW_WIDTH+DECODE_WIDTH

_idx                    = _raw_image+DECODE_WIDTH+(BAND_HEIGHT-5)*RAW_WIDTH ; 2
_blocks_rem_in_row      = _raw_image+DECODE_WIDTH+(BAND_HEIGHT-5)*RAW_WIDTH + 8 ; 1

_coef                   = _raw_image+DECODE_WIDTH+(BAND_HEIGHT-4)*RAW_WIDTH ; 128
.assert <_coef = COEF_LOW, error
.assert >(_coef+0) = >(_coef+128), error

_row_out                = _raw_image+DECODE_WIDTH+(BAND_HEIGHT-3)*RAW_WIDTH ; 128
_orig_y_table_l         = _raw_image+DECODE_WIDTH+(BAND_HEIGHT-2)*RAW_WIDTH ; BAND_HEIGHT
_orig_y_table_h         = _raw_image+DECODE_WIDTH+(BAND_HEIGHT-2)*RAW_WIDTH + BAND_HEIGHT

        .segment "LC"

.proc _mul473_m
                        .repeat 256, I
                          .byte ((I*473) .SHR 8) .BITAND $FF
                        .endrepeat
.endproc

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

.proc _normal_bits
                        .byte 8, 8, 8, 7, 7, 7, 7, 7
                        .byte 7, 7, 6, 5, 6, 7, 7, 7
                        .byte 6, 5, 5, 5, 5, 3, 0, 0
                        .byte 5, 5, 5, 6, 6, 5, 4, 4
                        .byte 0, 0, 0, 0, 0, 0, 0, 0
                        .byte 4, 4, 4, 0, 0, 0, 0, 0
                        .byte 0, 0, 0, 0, 0, 0, 0, 0
                        .byte 0, 0, 0, 0, 0, 0, 0, 0
.endproc

.proc _superfine_bits
                        .byte 10,10,10,9, 9, 9, 8, 8
                        .byte 8, 8, 7, 7, 7, 8, 8, 8
                        .byte 7, 7, 7, 7, 6, 5, 6, 6
                        .byte 7, 7, 7, 7, 7, 7, 6, 6
                        .byte 6, 6, 5, 4, 4, 4, 5, 6
                        .byte 6, 6, 6, 6, 6, 6, 5, 4
                        .byte 3, 3, 3, 5, 6, 6, 5, 4
                        .byte 3, 3, 3, 3, 4, 3, 3, 3
.endproc

block_step:             .byte 16, 8
row_step_l:             .byte <(8*RAW_WIDTH-DECODE_WIDTH+16), <(4*RAW_WIDTH-DECODE_WIDTH+8)
row_step_h:             .byte >(8*RAW_WIDTH-DECODE_WIDTH+16), >(4*RAW_WIDTH-DECODE_WIDTH+8)
