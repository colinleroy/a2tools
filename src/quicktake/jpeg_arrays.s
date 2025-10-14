; Too bad there's no #pragma align in cc65
        .export _ZAG_Coeff, _extendTests_l, _extendTests_h
        .export _extendOffsets_l, _extendOffsets_h
        .export _mul145_m, _mul145_l
        .export _mul217_m, _mul217_l
        .export _mul51_m, _mul51_l
        .export _mul106_m, _mul106_l
        .export _gWinogradQuant
        .export _gQuant0_l, _gQuant0_h, _gQuant1_l, _gQuant1_h
        .export _gHuffVal0, _gHuffVal1, _gHuffVal2, _gHuffVal3
        .export _gHuffTab0, _gHuffTab1, _gHuffTab2, _gHuffTab3
        .export _cache, _gCoeffBuf
        .export _raw_image
        .export right_shift_4
        .export _histogram_low, _histogram_high
        .export _orig_y_table_l, _orig_y_table_h
        .export _orig_x_offset, _special_x_orig_offset

.struct hufftable_t
   mMaxCode_l .res 16
   mMaxCode_h .res 16
   mValPtr    .res 16
   mGetMore   .res 16
.endstruct

        .segment "BSS"

; raw_image has 512px wide lines to help with alignment
; only the first 320 of each contain image data
; We'll fill in the blanks with the rest of the BSS data
; far enough that the scaler won't overwrite it (it will
; overwrite 256*16 bytes)
.align 256
_raw_image:       .res (BAND_HEIGHT-1)*RAW_WIDTH+DECODE_WIDTH

_gQuant0_l =      _raw_image+(10*512)+DECODE_WIDTH   ; 64 bytes
_gQuant0_h =      _raw_image+(11*512)+DECODE_WIDTH   ; 64 bytes
_gQuant1_l =      _raw_image+(12*512)+DECODE_WIDTH   ; 64 bytes
_gQuant1_h =      _raw_image+(13*512)+DECODE_WIDTH   ; 64 bytes

.assert .sizeof(hufftable_t) = 64, error
_gHuffTab0 =      _raw_image+(14*512)+DECODE_WIDTH   ; 64 bytes
_gHuffTab1 =      _raw_image+(15*512)+DECODE_WIDTH   ; 64 bytes
_gHuffTab2 =      _raw_image+(16*512)+DECODE_WIDTH   ; 64 bytes
_gHuffTab3 =      _raw_image+(17*512)+DECODE_WIDTH   ; 64 bytes
_gHuffVal0 =      _raw_image+(18*512)+DECODE_WIDTH   ; 16 bytes
_gHuffVal1 =      _raw_image+(19*512)+DECODE_WIDTH   ; 16 bytes

; Fill the last bytes to align cache
filler:                 .res RAW_WIDTH-DECODE_WIDTH-4
.assert <* = 256-4, error
_cache:                 .res CACHE_SIZE+4

.assert <* = 0, error
_gHuffVal2:             .res 256
_gHuffVal3:             .res 256

; Defined here to avoid holes due to alignment
_histogram_low:         .res 256
_histogram_high:        .res 256
_orig_x_offset:         .res 256
_special_x_orig_offset: .res 256

.assert <* = 0, error
_gCoeffBuf:             .res 128

_orig_y_table_l:        .res BAND_HEIGHT
_orig_y_table_h:        .res BAND_HEIGHT

        .segment "DATA"
.align 256
.proc _mul145_l
  .repeat 256, I
    .byte (I*145) .BITAND $FF
  .endrepeat
.endproc
.proc _mul145_m
  .repeat 256, I
    .byte ((I*145) .SHR 8) .BITAND $FF
  .endrepeat
.endproc
; .proc _mul145_h
  .repeat 256, I
    .assert ((I*145) .SHR 16) .BITAND $FF = 0, error
  .endrepeat
; .endproc

.proc _mul217_l
  .repeat 256, I
    .byte (I*217) .BITAND $FF
  .endrepeat
.endproc
.proc _mul217_m
  .repeat 256, I
    .byte ((I*217) .SHR 8) .BITAND $FF
  .endrepeat
.endproc
; .proc _mul217_h
  .repeat 256, I
    .assert ((I*217) .SHR 16) .BITAND $FF = 0, error
  .endrepeat
; .endproc

.proc _mul51_l
  .repeat 256, I
    .byte (I*51) .BITAND $FF
  .endrepeat
.endproc
.proc _mul51_m
  .repeat 256, I
    .byte ((I*51) .SHR 8) .BITAND $FF
  .endrepeat
.endproc
; .proc _mul51_h
  .repeat 256, I
    .assert ((I*51) .SHR 16) .BITAND $FF = 0, error
  .endrepeat
; .endproc

.proc _mul106_l
  .repeat 256, I
    .byte (I*106) .BITAND $FF
  .endrepeat
.endproc
.proc _mul106_m
  .repeat 256, I
    .byte ((I*106) .SHR 8) .BITAND $FF
  .endrepeat
.endproc
; .proc _mul106_h
  .repeat 256, I
    .assert ((I*106) .SHR 16) .BITAND $FF = 0, error
  .endrepeat
; .endproc

.assert <* = 0, error
.proc right_shift_4
  .repeat 256, I
    .byte I .SHR 4
  .endrepeat
.endproc

.assert <* = 0, error
START_LAST_ARRAYS = *
; Must be aligned to 0 (not only for page-crossing)
.proc _ZAG_Coeff
  .byte  $00
  .byte  $02
  .byte  $10
  .byte  $20
  .byte  $12
  .byte  $04
  .byte  $FF
  .byte  $14
  .byte  $22
  .byte  $FF
  .byte  $FF
  .byte  $FF
  .byte  $24
  .byte  $FF
  .byte  $FF
  .byte  $FF
  .byte  $FF
  .byte  $FF
  .byte  $FF
  .byte  $FF
  .byte  $FF
  .byte  $FF
  .byte  $FF
  .byte  $FF
  .byte  $FF
  .byte  $FF
  .byte  $FF
  .byte  $FF
  .byte  $FF
  .byte  $FF
  .byte  $FF
  .byte  $FF
  .byte  $FF
  .byte  $FF
  .byte  $FF
  .byte  $FF
  .byte  $FF
  .byte  $FF
  .byte  $FF
  .byte  $FF
  .byte  $FF
  .byte  $FF
  .byte  $FF
  .byte  $FF
  .byte  $FF
  .byte  $FF
  .byte  $FF
  .byte  $FF
  .byte  $FF
  .byte  $FF
  .byte  $FF
  .byte  $FF
  .byte  $FF
  .byte  $FF
  .byte  $FF
  .byte  $FF
  .byte  $FF
  .byte  $FF
  .byte  $FF
  .byte  $FF
  .byte  $FF
  .byte  $FF
  .byte  $FF
  .byte  $FF
.endproc

.proc _extendTests_l
  .byte  <$0000
  .byte  <$0001
  .byte  <$0002
  .byte  <$0004
  .byte  <$0008
  .byte  <$0010
  .byte  <$0020
  .byte  <$0040
  .byte  <$0080
  .byte  <$0100
  .byte  <$0200
  .byte  <$0400
  .byte  <$0800
  .byte  <$1000
  .byte  <$2000
  .byte  <$4000
.endproc
.proc _extendTests_h
  .byte  >$0000
  .byte  >$0001
  .byte  >$0002
  .byte  >$0004
  .byte  >$0008
  .byte  >$0010
  .byte  >$0020
  .byte  >$0040
  .byte  >$0080
  .byte  >$0100
  .byte  >$0200
  .byte  >$0400
  .byte  >$0800
  .byte  >$1000
  .byte  >$2000
  .byte  >$4000
.endproc
.proc _extendOffsets_l
  .byte  <(((-1)<<0) + 1)
  .byte  <(((-1)<<1) + 1)
  .byte  <(((-1)<<2) + 1)
  .byte  <(((-1)<<3) + 1)
  .byte  <(((-1)<<4) + 1)
  .byte  <(((-1)<<5) + 1)
  .byte  <(((-1)<<6) + 1)
  .byte  <(((-1)<<7) + 1)
  .byte  <(((-1)<<8) + 1)
  .byte  <(((-1)<<9) + 1)
  .byte  <(((-1)<<10) + 1)
  .byte  <(((-1)<<11) + 1)
  .byte  <(((-1)<<12) + 1)
  .byte  <(((-1)<<13) + 1)
  .byte  <(((-1)<<14) + 1)
  .byte  <(((-1)<<15) + 1)
.endproc
.proc _extendOffsets_h
  .byte  >(((-1)<<0) + 1)
  .byte  >(((-1)<<1) + 1)
  .byte  >(((-1)<<2) + 1)
  .byte  >(((-1)<<3) + 1)
  .byte  >(((-1)<<4) + 1)
  .byte  >(((-1)<<5) + 1)
  .byte  >(((-1)<<6) + 1)
  .byte  >(((-1)<<7) + 1)
  .byte  >(((-1)<<8) + 1)
  .byte  >(((-1)<<9) + 1)
  .byte  >(((-1)<<10) + 1)
  .byte  >(((-1)<<11) + 1)
  .byte  >(((-1)<<12) + 1)
  .byte  >(((-1)<<13) + 1)
  .byte  >(((-1)<<14) + 1)
  .byte  >(((-1)<<15) + 1)
.endproc
END_ARRAYS = *
.assert >START_LAST_ARRAYS = >END_ARRAYS, error ; align that better!

.align 256
.proc _gWinogradQuant
  .byte  $80
  .byte  $B2
  .byte  $B2
  .byte  $A7
  .byte  $F6
  .byte  $A7
  .byte  $97
  .byte  $E8
  .byte  $E8
  .byte  $97
  .byte  $80
  .byte  $D1
  .byte  $DB
  .byte  $D1
  .byte  $80
  .byte  $65
  .byte  $B2
  .byte  $C5
  .byte  $C5
  .byte  $B2
  .byte  $65
  .byte  $45
  .byte  $8B
  .byte  $A7
  .byte  $B1
  .byte  $A7
  .byte  $8B
  .byte  $45
  .byte  $23
  .byte  $60
  .byte  $83
  .byte  $97
  .byte  $97
  .byte  $83
  .byte  $60
  .byte  $23
  .byte  $31
  .byte  $5B
  .byte  $76
  .byte  $80
  .byte  $76
  .byte  $5B
  .byte  $31
  .byte  $2E
  .byte  $51
  .byte  $65
  .byte  $65
  .byte  $51
  .byte  $2E
  .byte  $2A
  .byte  $45
  .byte  $4F
  .byte  $45
  .byte  $2A
  .byte  $23
  .byte  $36
  .byte  $36
  .byte  $23
  .byte  $1C
  .byte  $25
  .byte  $1C
  .byte  $13
  .byte  $13
  .byte  $0A
.endproc
