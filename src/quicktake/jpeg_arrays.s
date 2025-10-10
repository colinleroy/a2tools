; Too bad there's no #pragma align in cc65
        .export _ZAG_Coeff, _extendTests_l, _extendTests_h
        .export _extendOffsets_l, _extendOffsets_h
        .export _mul362_h, _mul362_m, _mul362_l
        .export _mul473_h, _mul473_m, _mul473_l
        .export _mul196_m, _mul196_l
        .export _gWinogradQuant
        .export _gQuant0_l, _gQuant0_h, _gQuant1_l, _gQuant1_h
        .export _gHuffVal0, _gHuffVal1, _gHuffVal2, _gHuffVal3
        .export _gHuffTab0, _gHuffTab1, _gHuffTab2, _gHuffTab3
        .export _gMCUBufG, _cache, _gCoeffBuf
        .export right_shift_4


.struct hufftable_t
   mMaxCode_l .res 16
   mMaxCode_h .res 16
   mValPtr    .res 16
   mGetMore   .res 16
.endstruct

        .segment "BSS"

.align 256
_gQuant0_l:       .res 64
_gQuant0_h:       .res 64
_gQuant1_l:       .res 64
_gQuant1_h:       .res 64

.assert <* = 0, error
_gHuffVal2:       .res 256
_gHuffVal3:       .res 256

.assert <* = 0, error
_gMCUBufG:        .res 128
_gCoeffBuf:       .res 128

.assert <* = 0, error
_gHuffTab0:       .res .sizeof(hufftable_t)
_gHuffTab1:       .res .sizeof(hufftable_t)
_gHuffTab2:       .res .sizeof(hufftable_t)
_gHuffTab3:       .res .sizeof(hufftable_t)

.assert <* = 0, error
_gHuffVal0:       .res 16
_gHuffVal1:       .res 16

filler:           .res 256-32-4
.assert <* = 256-4, error
_cache:           .res CACHE_SIZE+4

        .segment "DATA"
; 512 + 128 + 16 + 8 + 4 + 1  - too much shifting
.align 256
; 256+64+32+8+2 - too much shifting
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
.proc _mul362_h
  .repeat 256, I
    .byte ((I*362) .SHR 16) .BITAND $FF
  .endrepeat
.endproc

;256+16+4+1 - too much shifting

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
.proc _mul473_h
  .repeat 256, I
    .byte ((I*473) .SHR 16) .BITAND $FF
  .endrepeat
.endproc

; 128 + 64 + 4 - shifting also longer than table lookup
.proc _mul196_l
  .repeat 256, I
    .byte (I*196) .BITAND $FF
  .endrepeat
.endproc
.proc _mul196_m
  .repeat 256, I
    .byte ((I*196) .SHR 8) .BITAND $FF
  .endrepeat
.endproc

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
  .byte  <$FFFF
  .byte  <$FFFE
  .byte  <$FFFC
  .byte  <$FFF8
  .byte  <$FFF0
  .byte  <$FFE0
  .byte  <$FFC0
  .byte  <$FF80
  .byte  <$FF00
  .byte  <$FE00
  .byte  <$FC00
  .byte  <$F800
  .byte  <$F000
  .byte  <$E000
  .byte  <$C000
  .byte  <$8000
.endproc
.proc _extendOffsets_h
  .byte  >$FFFF
  .byte  >$FFFE
  .byte  >$FFFC
  .byte  >$FFF8
  .byte  >$FFF0
  .byte  >$FFE0
  .byte  >$FFC0
  .byte  >$FF80
  .byte  >$FF00
  .byte  >$FE00
  .byte  >$FC00
  .byte  >$F800
  .byte  >$F000
  .byte  >$E000
  .byte  >$C000
  .byte  >$8000
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
