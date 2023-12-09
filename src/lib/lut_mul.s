        .export         _lut_mul
        .export         _lut_smul
        .import         popa, negax


; Wicked fast LUT-based multiplication
; input: a = factor a; x = factor b
; output: mul_factor_a = factor a; mul_factor_x = x = factor x;
;         mul_product_lo = low byte of product a*b;
;         mul_product_hi = a = high byte of product
; preserved: x, y
; max cycles: under 90
;

_lut_mul:
  ldx #0
  stx neg
  sta mul_factor_x      ; setup: 6 cycles
  jsr popa
  sta mul_factor_a
  bra mul

_lut_smul:
  ldx #0
  stx neg
  sta mul_factor_x
  and #$80
  beq check_a
  ldx #1
  stx neg
  lda mul_factor_x
  eor #$FF
  clc
  adc #1
  sta mul_factor_x

check_a:
  jsr popa
  sta mul_factor_a
  and #$80
  beq reload_mul
  ldx #1
  cpx neg
  bne was_not_neg
  ldx #0
was_not_neg:
  stx neg
  lda mul_factor_a
  eor #$FF
  clc
  adc #1
  sta mul_factor_a

reload_mul:
  lda mul_factor_a
mul:
  clc                   ; (a + x)^2/2: 23 cycles
  adc mul_factor_x
  tax
  bcc cclear
  lda mul_hibyte512,x
  bcs b
cclear:
  lda mul_hibyte256,x
  sec
b:
  sta mul_product_hi
  lda mul_lobyte256,x

  ldx mul_factor_a      ; - a^2/2: 20 cycles
  sbc mul_lobyte256,x
  sta mul_product_lo
  lda mul_product_hi
  sbc mul_hibyte256,x
  sta mul_product_hi

  ldx mul_factor_x      ; + x & a & 1: 22 cycles
  txa                   ; (this is a kludge to correct a
  and mul_factor_a      ; roundoff error that makes odd * odd too low)
  and #1

  clc
  adc mul_product_lo
  bcc c
  inc mul_product_hi
c:
  sec                   ; - x^2/2: 25 cycles
  sbc mul_lobyte256,x
  tay
  lda mul_product_hi
  sbc mul_hibyte256,x
  tax
  tya
; check sign
  ldy #1
  cpy neg
  bne done
  jsr negax
done:
  rts

.rodata
; here are the big fat lookup tables
mul_lobyte256:
  .byte   0,  1,  2,  5,  8, 13, 18, 25, 32, 41, 50, 61, 72, 85, 98,113
  .byte 128,145,162,181,200,221,242,  9, 32, 57, 82,109,136,165,194,225
  .byte   0, 33, 66,101,136,173,210,249, 32, 73,114,157,200,245, 34, 81
  .byte 128,177,226, 21, 72,125,178,233, 32, 89,146,205,  8, 69,130,193
  .byte   0, 65,130,197,  8, 77,146,217, 32,105,178,253, 72,149,226, 49
  .byte 128,209, 34,117,200, 29,114,201, 32,121,210, 45,136,229, 66,161
  .byte   0, 97,194, 37,136,237, 82,185, 32,137,242, 93,200, 53,162, 17
  .byte 128,241, 98,213, 72,189, 50,169, 32,153, 18,141,  8,133,  2,129
  .byte   0,129,  2,133,  8,141, 18,153, 32,169, 50,189, 72,213, 98,241
  .byte 128, 17,162, 53,200, 93,242,137, 32,185, 82,237,136, 37,194, 97
  .byte   0,161, 66,229,136, 45,210,121, 32,201,114, 29,200,117, 34,209
  .byte 128, 49,226,149, 72,253,178,105, 32,217,146, 77,  8,197,130, 65
  .byte   0,193,130, 69,  8,205,146, 89, 32,233,178,125, 72, 21,226,177
  .byte 128, 81, 34,245,200,157,114, 73, 32,249,210,173,136,101, 66, 33
  .byte   0,225,194,165,136,109, 82, 57, 32,  9,242,221,200,181,162,145
  .byte 128,113, 98, 85, 72, 61, 50, 41, 32, 25, 18, 13,  8,  5,  2,  1
mul_hibyte256:
  .byte   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0
  .byte   0,  0,  0,  0,  0,  0,  0,  1,  1,  1,  1,  1,  1,  1,  1,  1
  .byte   2,  2,  2,  2,  2,  2,  2,  2,  3,  3,  3,  3,  3,  3,  4,  4
  .byte   4,  4,  4,  5,  5,  5,  5,  5,  6,  6,  6,  6,  7,  7,  7,  7
  .byte   8,  8,  8,  8,  9,  9,  9,  9, 10, 10, 10, 10, 11, 11, 11, 12
  .byte  12, 12, 13, 13, 13, 14, 14, 14, 15, 15, 15, 16, 16, 16, 17, 17
  .byte  18, 18, 18, 19, 19, 19, 20, 20, 21, 21, 21, 22, 22, 23, 23, 24
  .byte  24, 24, 25, 25, 26, 26, 27, 27, 28, 28, 29, 29, 30, 30, 31, 31
  .byte  32, 32, 33, 33, 34, 34, 35, 35, 36, 36, 37, 37, 38, 38, 39, 39
  .byte  40, 41, 41, 42, 42, 43, 43, 44, 45, 45, 46, 46, 47, 48, 48, 49
  .byte  50, 50, 51, 51, 52, 53, 53, 54, 55, 55, 56, 57, 57, 58, 59, 59
  .byte  60, 61, 61, 62, 63, 63, 64, 65, 66, 66, 67, 68, 69, 69, 70, 71
  .byte  72, 72, 73, 74, 75, 75, 76, 77, 78, 78, 79, 80, 81, 82, 82, 83
  .byte  84, 85, 86, 86, 87, 88, 89, 90, 91, 91, 92, 93, 94, 95, 96, 97
  .byte  98, 98, 99,100,101,102,103,104,105,106,106,107,108,109,110,111
  .byte 112,113,114,115,116,117,118,119,120,121,122,123,124,125,126,127
mul_hibyte512:
  .byte 128,129,130,131,132,133,134,135,136,137,138,139,140,141,142,143
  .byte 144,145,146,147,148,149,150,152,153,154,155,156,157,158,159,160
  .byte 162,163,164,165,166,167,168,169,171,172,173,174,175,176,178,179
  .byte 180,181,182,184,185,186,187,188,190,191,192,193,195,196,197,198
  .byte 200,201,202,203,205,206,207,208,210,211,212,213,215,216,217,219
  .byte 220,221,223,224,225,227,228,229,231,232,233,235,236,237,239,240
  .byte 242,243,244,246,247,248,250,251,253,254,255,  1,  2,  4,  5,  7
  .byte   8,  9, 11, 12, 14, 15, 17, 18, 20, 21, 23, 24, 26, 27, 29, 30
  .byte  32, 33, 35, 36, 38, 39, 41, 42, 44, 45, 47, 48, 50, 51, 53, 54
  .byte  56, 58, 59, 61, 62, 64, 65, 67, 69, 70, 72, 73, 75, 77, 78, 80
  .byte  82, 83, 85, 86, 88, 90, 91, 93, 95, 96, 98,100,101,103,105,106
  .byte 108,110,111,113,115,116,118,120,122,123,125,127,129,130,132,134
  .byte 136,137,139,141,143,144,146,148,150,151,153,155,157,159,160,162
  .byte 164,166,168,169,171,173,175,177,179,180,182,184,186,188,190,192
  .byte 194,195,197,199,201,203,205,207,209,211,212,214,216,218,220,222
  .byte 224,226,228,230,232,234,236,238,240,242,244,246,248,250,252,254
  
.bss
mul_factor_a: .res 1
mul_factor_x: .res 1
mul_product_lo: .res 1
mul_product_hi: .res 1
neg: .res 1
