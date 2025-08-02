
        .importzp   tmp1, tmp2, tmp3, tmp4, ptr1, ptr2, ptr3, _prev_ram_irq_vector, _prev_rom_irq_vector, c_sp, ptr4
        .importzp   _zp2, _zp4, _zp6, _zp8, _zp9, _zp10, _zp11, _zp12, _zp13
        .import     popptr1
        .import     _extendTests, _extendOffsets
        .import     _cache_end, _fillInBuf
        .import     _mul669_l, _mul669_m, _mul669_h
        .import     _mul362_l, _mul362_m, _mul362_h
        .import     _mul277_l, _mul277_m, _mul277_h
        .import     _mul196_l, _mul196_m, _mul196_h
        .import     _gCoeffBuf, _gRestartInterval, _gRestartsLeft
        .import     _gMaxBlocksPerMCU, _processRestart, _gCompACTab, _gCompQuant
        .import     _gQuant0_l, _gQuant1_l, _gQuant0_h, _gQuant1_h
        .import     _gCompDCTab, _gMCUOrg, _gLastDC, _gCoeffBuf, _ZAG_Coeff
        .import     _gHuffTab0, _gHuffVal0, _gHuffTab1, _gHuffVal1, _gHuffTab2, _gHuffVal2, _gHuffTab3, _gHuffVal3
        .import     _gMCUBufG
        .import     _gNumMCUSRemainingX, _gNumMCUSRemainingY
        .import     shraxy, decsp4, popax, addysp, mult16x16x16_direct
        .export     _huffExtend, _getBits1, _getBits2
        .export     _imul_b1_b3, _imul_b2, _imul_b4, _imul_b5
        .export     _idctRows, _idctCols, _decodeNextMCU, _transformBlock
        .export     _pjpeg_decode_mcu, _skipVariableMarker

; ZP vars. Mind that qt-conv uses some too
_gBitBuf       = _zp2       ; word, used everywhere
code           = _zp4       ; word, used in huffDecode
cur_gMCUOrg    = _zp6       ; word, used in _decodeNextMCU
_gBitsLeft     = _zp8       ; byte, used everywhere

cur_pQ         = _zp9       ; byte, used in _decodeNextMCU
cur_ZAG_coeff  = _zp10      ; byte, used in _decodeNextMCU
rDMCU          = _zp11      ; byte, used in _decodeNextMCU
sDMCU          = _zp12      ; byte, used in _decodeNextMCU
iDMCU          = _zp13      ; byte, used in _decodeNextMCU

dw             = _zp9       ; byte, used in imul (IDCT)
neg            = _zp10      ; byte, used in imul (IDCT)

_cur_cache_ptr = _prev_ram_irq_vector

.struct hufftable_t
   mMinCode .res 32
   mMaxCode .res 32
   mValPtr  .res 32
.endstruct

.macro INLINE_ASRAX7
.scope
        asl                     ;          AAAAAAA0, h->C
        txa
        rol                     ;          XXXXXXLh, H->C
        ldx     #$00            ; 00000000 XXXXXXLh
        bcc     @done
        dex                     ; 11111111 XXXXXXLh if C
@done:
.endscope
.endmacro

; uint8 getBit(void)
; Returns with A = 0 and carry set if bit 1
.macro INLINE_GETBIT
.scope
        dec     _gBitsLeft
        bpl     haveBit

        sty     tmp3          ; Backup Y
        ldy     #$FF
        jsr     getOctet
        ldy     tmp3
        asl     a
        sta     _gBitBuf

        lda     #7
        sta     _gBitsLeft
        jmp     done

haveBit:
        asl     _gBitBuf
done:
        rol     _gBitBuf+1    ; Sets carry
        lda     #0
.endscope
.endmacro

; PJPG_INLINE int16 __fastcall__ huffExtend(uint16 x, uint8 sDMCU)

_huffExtend:
        cmp     #16
        bcc     :+

retNormal:
        lda     extendX
        ldx     extendX+1
        rts

:       asl     a
        sta     tmp1
        tay
        ldx     _extendTests+1,y

        cpx     extendX+1
        bcc     retNormal
        bne     retExtend

        lda     _extendTests,y
        cmp     extendX
        bcc     retNormal
        beq     retNormal

retExtend:
        ldx     tmp1
        lda     _extendOffsets,x
        clc
        adc     extendX
        tay
        lda     _extendOffsets+1,x
        adc     extendX+1
        tax
        tya
        rts

; #define getBits1(n) getBits(n, 0)
; #define getBits2(n) getBits(n, 1)

sixteen_min_n: .byte 16
               .byte 15
               .byte 14
               .byte 13
               .byte 12
               .byte 11
               .byte 10
               .byte 9
eight_min_n:
               .byte 8
               .byte 7
               .byte 6
               .byte 5
               .byte 4
               .byte 3
               .byte 2
               .byte 1
               .byte 0

n_min_eight:   .byte 0
               .byte 0
               .byte 0
               .byte 0
               .byte 0
               .byte 0
               .byte 0
               .byte 0
               .byte 0
               .byte 1
               .byte 2
               .byte 3
               .byte 4
               .byte 5
               .byte 6
               .byte 7
               .byte 8

_getBits1:
        ldx     #0
        beq     getBits
_getBits2:
        ldx     #$FF
getBits:
        stx     ff
        sta     n
        tay
        lda     sixteen_min_n,y
        sta     final_shift

        lda     _gBitBuf
        sta     ret
        lda     _gBitBuf+1
        sta     ret+1

        ldy     n
        cpy     #9
        bcc     n_lt8

        lda     n_min_eight,y
        sta     n

        ldy     _gBitsLeft
        beq     no_lshift

        ; no need to check for << 8, that can't be, as _gBitsLeft maximum is 7
        lda     _gBitBuf

:       asl     a
        rol     _gBitBuf+1
        dey
        bne     :-
        ;sta     _gBitBuf - will be overwritten

no_lshift:
        ldy     ff
        jsr     getOctet
        sta     _gBitBuf

        ldx     _gBitsLeft
        ldy     eight_min_n,x
        beq     no_lshift2
        cpy     #8
        bne     :+

        lda     _gBitBuf
        sta     _gBitBuf+1
        ; lda     #$00      - no need to store, getOctet'd later
        ; sta     _gBitBuf
        beq     no_lshift2

:       lda     _gBitBuf
:       asl     a
        rol     _gBitBuf+1
        dey
        bne     :-
        sta     _gBitBuf
        lda     _gBitBuf+1

no_lshift2:
        sta     ret

n_lt8:
        ldy     _gBitsLeft
        beq     no_lshift3
        cpy     n
        bcs     enoughBits

        ; no need to check for << 8, that can't be
        lda     _gBitBuf
:       asl     a
        rol     _gBitBuf+1
        dey
        bne     :-
        ;sta     _gBitBuf - will get overwritten by getOctet

no_lshift3:
        ldy     ff
        jsr     getOctet
        sta     _gBitBuf

        lda     n
        sec
        sbc     _gBitsLeft
        tay
        beq     no_lshift4

        cmp     #8
        bne     :+

        lda     _gBitBuf
        sta     _gBitBuf+1
        ; lda     #$00         - no need to store, will be getOctet'd
        ; sta     _gBitBuf
        jmp     no_lshift4

:       tax
        lda     _gBitBuf
:       asl     a
        rol     _gBitBuf+1
        dex
        bne     :-
        sta     _gBitBuf

no_lshift4:
        lda     eight_min_n,y
        sta     _gBitsLeft
        jmp     no_lshift5

enoughBits:
        lda     _gBitsLeft
        sec
        sbc     n
        sta     _gBitsLeft

        ldy     n
        beq     no_lshift5
        cpy     #8
        bne     :+
        lda     _gBitBuf
        sta     _gBitBuf+1
        ; lda     #$00      - no need to store, will be getOctet'd
        ; sta     _gBitBuf
        jmp     no_lshift5

:       lda     _gBitBuf
:       asl     a
        rol     _gBitBuf+1
        dey
        bne     :-
        sta     _gBitBuf

no_lshift5:
        ldy     final_shift
        cpy     #8
        bcs     :+
        lda     ret
        ldx     ret+1
        jmp     shraxy

:       ldx     n_min_eight,y
        beq     :++
        lda     ret+1
:       lsr     a
        dex
        bne     :-
        ; Now X is 0
        ; ldx   #0
        rts
:
        lda     ret+1
        ; ldx   #0
        rts

;uint8 getOctet(uint8 FFCheck)
;warning: param in Y

check_cache_high1:
        lda     _cur_cache_ptr+1
        cmp     _cache_end+1
        bne     continue1
        sty     tmp1
        stx     tmp4
        jsr     _fillInBuf
        ldy     tmp1
        ldx     tmp4
        jmp     continue1

check_cache_high2:
        lda     _cur_cache_ptr+1
        cmp     _cache_end+1
        bne     continue3
        sty     tmp1
        stx     tmp4
        jsr     _fillInBuf
        ldy     tmp1
        ldx     tmp4
        jmp     continue3

inc_cache_high1:
        inc     _cur_cache_ptr+1
        jmp     continue2

inc_cache_high2:
        inc     _cur_cache_ptr+1
        jmp     continue4


getOctet:
        sty     ffcheck         ; Backup param
        ldy     #$00
        lda     _cur_cache_ptr
        cmp     _cache_end
        beq     check_cache_high1

continue1:
        ; Load char from buffer
        lda     (_cur_cache_ptr),y
        ; Increment buffer pointer
        inc     _cur_cache_ptr
        beq     inc_cache_high1

continue2:
        ; Should we check for $FF?
        bit     ffcheck
        bmi     :+
        rts
:       cmp     #$FF          ; Is result FF?
        beq     :+
        rts
:       pha                     ; Remember result
        ; Yes. Read again.
        lda     _cur_cache_ptr
        cmp     _cache_end
        beq     check_cache_high2

continue3:
        lda     (_cur_cache_ptr),y
        inc     _cur_cache_ptr
        beq     inc_cache_high2

continue4:
        cmp     #$00
        beq     out

        ; Stuff back chars
        sta     (_cur_cache_ptr),y
        lda     _cur_cache_ptr
        sec
        sbc     #2
        bcs     :+
        dec     _cur_cache_ptr+1

:       iny
        lda     #$FF
        sta     (_cur_cache_ptr),y

out:
        ; Return result
        pla
        rts

.macro imul TABL, TABM, TABH
.scope
        ldy     #$00
        sty     neg
        tay                   ; val low byte in Y

        cpx     #$80
        bcc     :+
        stx     neg

        ; val = -val;
        clc
        eor     #$FF
        adc     #1
        tay
        txa
        eor    #$FF
        adc    #0
        tax

:       stx    tmp4
        ; dw = mul362_l[l] | mul362_m[l] <<8 | mul362_h[l] <<16;
        lda    TABL,y
        sta    dw
        ; lda    _mul362_m,y - shortcut right below
        ; sta    tmp1
        ; lda    _mul362_h,y
        ; sta    tmp2

        ; dw += (mul362_l[h]) << 8;
        clc
        lda    TABM,y    ; tmp1
        ldx    tmp4
        adc    TABL,x
        sta    tmp1

        ; dw += (mul362_m[h]) << 16;
        .if .paramcount = 3
        lda    TABH,y
        .else
        lda    #$00
        .endif
        adc    TABM,x
        sta    tmp2

        ; Was val negative?
        ldy    neg
        bne    :+
        tax
        lda    tmp1
        rts
:
        ; dw ^= 0xffffffff
        lda    #$FF
        eor    dw
        sta    dw
        lda    #$FF
        eor    tmp2
        tax
        lda    #$FF
        eor    tmp1

        ; dw++;
        inc    dw
        bne    :+
        clc
        adc    #1
        bne    :+
        inx
:       rts
.endscope
.endmacro

; uint16 __fastcall__ imul_b1_b3(int16 w)
_imul_b1_b3:
        imul    _mul362_l, _mul362_m, _mul362_h

; uint16 __fastcall__ imul_b2(int16 w)

_imul_b2:
        imul    _mul669_l, _mul669_m, _mul669_h

; uint16 __fastcall__ imul_b4(int16 w)
_imul_b4:
        imul    _mul277_l, _mul277_m, _mul277_h

; uint16 __fastcall__ imul_b5(int16 w)
_imul_b5:
        imul    _mul196_l, _mul196_m
        ldy     #$00
        sty     neg
        tay             ; val low byte in Y

        cpx     #$80
        bcc     :+
        stx     neg

        ; val = -val;
        clc
        eor     #$FF
        adc     #1
        tay
        txa
        eor    #$FF
        adc    #0
        tax

:
        stx     tmp4
        ; dw = mul196_l[l] | mul196_m[l] <<8 | mul196_h[l] <<16;
        lda    _mul196_l,y
        sta    dw
        ; lda    _mul196_m,y - shortcut right below
        ; sta    tmp1

        ; Useless (0)
        ; lda    _mul196_h,y
        ; sta    tmp2

        ; dw += (mul196_l[h]) << 8;
        clc
        lda    _mul196_m,y    ; tmp1
        ldx    tmp4
        adc    _mul196_l,x
        sta    tmp1

        ; dw += (mul196_m[h]) << 16;
        lda    #0
        adc    _mul196_m,x
        sta    tmp2

        ; Was val negative?
        ldy    neg
        bne    :+
        tax
        lda    tmp1
        rts
:
        ; dw ^= 0xffffffff
        lda    #$FF
        eor    dw
        sta    dw
        lda    #$FF
        eor    tmp2
        tax
        lda    #$FF
        eor    tmp1

        ; dw++;
        inc    dw
        bne    :+
        clc
        adc    #1
        bne    :+
        inx
:       rts

; uint8 __fastcall__ huffDecode(const uint8* pHuffVal)
.macro huffDecode TABLE, VAL
.scope
        INLINE_GETBIT
        sta     code+1  ; A = 0 here
        rol             ; Now A = 0 or 1 depending on bit
        sta     code

        ldx     #16
        ldy     #$FE
nextLoop:
        ; *curMaxCode != 0xFFFF?
        iny                     ; FF - 1 - 3 ...
        iny                     ; 0  - 2 - 4 ...

        lda TABLE+1+32,y        ; curMaxCode == 0xFFFF? hibyte
        cmp #$FF
        beq checkLow

        cmp code+1              ; curMaxCode < code ? hibyte
        bcc noTest
        bne loopDone

checkLow:
        lda TABLE+32,y
        cmp #$FF                ; curMaxCode == 0xFFFF? lobyte
        beq noTest
        cmp code                ; ; curMaxCode < code ? lobyte
        bcs loopDone
noTest:
        INLINE_GETBIT
        rol     code
        rol     code+1
        dex
        bne     nextLoop
        rts
loopDone:
        clc
        lda     TABLE+64,y
        adc     code
        sec
        sbc     TABLE,y
        tay                     ; Get index

        lda     VAL,y
        rts
.endscope
.endmacro

_huffDecode0:
        huffDecode _gHuffTab0, _gHuffVal0

_huffDecode1:
        huffDecode _gHuffTab1, _gHuffVal1

_huffDecode2:
        huffDecode _gHuffTab2, _gHuffVal2

_huffDecode3:
        huffDecode _gHuffTab3, _gHuffVal3

; void idctRows(void

_idctRows:
        lda    #8
        sta    idctRC

        ldy    #0
nextIdctRowsLoop:

        lda    _gCoeffBuf+2,y
        bne    full_idct_rows
        lda    _gCoeffBuf+4,y
        bne    full_idct_rows
        lda    _gCoeffBuf+6,y
        bne    full_idct_rows
        lda    _gCoeffBuf+8,y
        bne    full_idct_rows
        lda    _gCoeffBuf+10,y
        bne    full_idct_rows
        lda    _gCoeffBuf+12,y
        bne    full_idct_rows
        lda    _gCoeffBuf+14,y
        bne    full_idct_rows

        lda    _gCoeffBuf+3,y
        bne    full_idct_rows
        lda    _gCoeffBuf+5,y
        bne    full_idct_rows
        lda    _gCoeffBuf+7,y
        bne    full_idct_rows
        lda    _gCoeffBuf+9,y
        bne    full_idct_rows
        lda    _gCoeffBuf+11,y
        bne    full_idct_rows
        lda    _gCoeffBuf+13,y
        bne    full_idct_rows
        lda    _gCoeffBuf+15,y
        bne    full_idct_rows

        ; Short circuit the 1D IDCT if only the DC component is non-zero
        lda    _gCoeffBuf,y
        sta    _gCoeffBuf+4,y
        sta    _gCoeffBuf+8,y
        sta    _gCoeffBuf+12,y

        lda    _gCoeffBuf+1,y
        sta    _gCoeffBuf+5,y
        sta    _gCoeffBuf+9,y
        sta    _gCoeffBuf+13,y

        jmp    cont_idct_rows

full_idct_rows:

        clc
        lda    _gCoeffBuf+10,y
        adc    _gCoeffBuf+6,y
        sta    x7
        lda    _gCoeffBuf+11,y
        tax
        adc    _gCoeffBuf+7,y
        sta    x7+1

        sec
        lda    _gCoeffBuf+10,y
        sbc    _gCoeffBuf+6,y
        sta    x4
        txa
        sbc    _gCoeffBuf+7,y
        sta    x4+1

        clc
        lda    _gCoeffBuf+2,y
        adc    _gCoeffBuf+14,y
        sta    x5
        lda    _gCoeffBuf+3,y
        tax
        adc    _gCoeffBuf+15,y
        sta    x5+1

        sec
        lda    _gCoeffBuf+2,y
        sbc    _gCoeffBuf+14,y
        sta    x6
        txa
        sbc    _gCoeffBuf+15,y
        sta    x6+1

        clc
        lda    _gCoeffBuf,y
        adc    _gCoeffBuf+8,y
        sta    x30
        lda    _gCoeffBuf+1,y
        tax
        adc    _gCoeffBuf+9,y
        sta    x30+1

        sec
        lda    _gCoeffBuf,y
        sbc    _gCoeffBuf+8,y
        sta    x31
        txa
        sbc    _gCoeffBuf+9,y
        sta    x31+1

        clc
        lda    _gCoeffBuf+4,y
        adc    _gCoeffBuf+12,y
        sta    x13
        lda    _gCoeffBuf+5,y
        tax
        adc    _gCoeffBuf+13,y
        sta    x13+1

        ; x12 = *(rowSrc_2) - *(rowSrc_6);
        sec
        lda    _gCoeffBuf+4,y
        sbc    _gCoeffBuf+12,y
        pha
        txa
        sbc    _gCoeffBuf+13,y

        ; x32 = imul_b1_b3(x12) - x13;
        tax
        pla
        sty     tmp3
        jsr    _imul_b1_b3
        ldy     tmp3
        sec
        sbc    x13
        sta    x32
        txa
        sbc    x13+1
        sta    x32+1

        clc
        lda    x5
        adc    x7
        sta    x17
        pha
        lda    x5+1
        adc    x7+1
        sta    x17+1
        tax

        ;*(rowSrc) = x30 + x13 + x17;
        clc
        pla                     ; x17
        adc    x13
        pha
        txa
        adc    x13+1
        tax
        pla
        clc
        adc    x30
        sta    _gCoeffBuf,y
        txa
        adc    x30+1
        sta    _gCoeffBuf+1,y

        sty     tmp3

        lda    x4
        sec
        sbc    x6
        tay
        lda    x4+1
        sbc    x6+1
        tax
        tya
        jsr    _imul_b5
        sta    res1
        stx    res1+1

        lda    x4
        ldx    x4+1
        jsr    _imul_b2
        sta    tmp1
        stx    tmp2
        lda    res1
        sec
        sbc    tmp1
        sta    x24
        lda    res1+1
        sbc    tmp2
        sta    x24+1

        lda    x6
        ldx    x6+1
        jsr    _imul_b4
        sec
        sbc    res1
        tay                     ; stg26
        txa
        sbc    res1+1
        tax

        tya                     ; stg26
        sec
        sbc    x17
        sta    res2
        txa                     ; stg26+1
        sbc    x17+1
        sta    res2+1

        ; x15 = x5 - x7;
        sec
        lda    x5
        sbc    x7
        pha
        lda    x5+1
        sbc    x7+1
        tax

        ; res3 = imul_b1_b3(x15) - res2;
        pla
        jsr    _imul_b1_b3
        sec
        sbc    res2
        sta    res3
        tay
        txa
        sbc    res2+1
        sta    res3+1
        tax

        ; *(rowSrc_2) = x31 - x32 + res3;
        tya
        clc
        adc    x31
        tay
        txa
        adc    x31+1
        tax
        tya
        sec
        sbc    x32
        ldy     tmp3
        sta    _gCoeffBuf+4,y
        txa
        sbc    x32+1
        sta    _gCoeffBuf+5,y

        ; *(rowSrc_4) = x30 + res3 + x24 - x13;
        sty     tmp3
        lda    x30
        clc
        adc    res3
        tay
        lda    x30+1
        adc    res3+1
        tax
        tya
        adc    x24
        tay
        txa
        adc    x24+1
        tax
        tya
        sec
        sbc    x13
        ldy     tmp3
        sta    _gCoeffBuf+8,y
        txa
        sbc    x13+1
        sta    _gCoeffBuf+9,y

        ; *(rowSrc_6) = x31 + x32 - res2;
        sty     tmp3
        lda    x31
        clc
        adc    x32
        tay
        lda    x31+1
        adc    x32+1
        tax
        tya
        sec
        sbc    res2
        ldy     tmp3
        sta    _gCoeffBuf+12,y
        txa
        sbc    res2+1
        sta    _gCoeffBuf+13,y

cont_idct_rows:
        dec    idctRC
        beq    :+
        clc
        tya
        adc    #16
        tay
        jmp    nextIdctRowsLoop

:       rts

; void idctCols(void

_idctCols:
        lda     #8
        sta     idctCC

        ldy     #0
nextCol:
        lda     _gCoeffBuf+32,y
        bne     full_idct_cols
        lda     _gCoeffBuf+64,y
        bne     full_idct_cols
        lda     _gCoeffBuf+96,y
        bne     full_idct_cols
        lda     _gCoeffBuf+33,y
        bne     full_idct_cols
        lda     _gCoeffBuf+65,y
        bne     full_idct_cols
        lda     _gCoeffBuf+97,y
        bne     full_idct_cols

        ; Short circuit the 1D IDCT if only the DC component is non-zero
        ldx     _gCoeffBuf+1,y
        lda     _gCoeffBuf,y

        INLINE_ASRAX7

        clc
        adc     #$80
        bcc     :+
        inx
:       cpx     #$80
        bcc     :+
        lda     #0
        beq     clampDone1
:       cpx     #0
        beq     clampDone1
        lda     #$FF

clampDone1:
        sta     _gCoeffBuf,y
        sta     _gCoeffBuf+32,y
        sta     _gCoeffBuf+64,y
        sta     _gCoeffBuf+96,y
        lda     #$00
        sta     _gCoeffBuf+1,y
        sta     _gCoeffBuf+33,y
        sta     _gCoeffBuf+65,y
        sta     _gCoeffBuf+97,y
        jmp     cont_idct_cols

full_idct_cols:
        sec
        lda     _gCoeffBuf+80,y
        sbc     _gCoeffBuf+48,y
        sta     x4
        lda     _gCoeffBuf+81,y
        tax
        sbc     _gCoeffBuf+49,y
        sta     x4+1

        clc
        lda     _gCoeffBuf+80,y
        adc     _gCoeffBuf+48,y
        sta     x7
        txa
        adc     _gCoeffBuf+49,y
        sta     x7+1

        sec
        lda     _gCoeffBuf+16,y
        sbc     _gCoeffBuf+112,y
        sta     x6
        lda     _gCoeffBuf+17,y
        tax
        sbc     _gCoeffBuf+113,y
        sta     x6+1

        clc
        lda     _gCoeffBuf+16,y
        adc     _gCoeffBuf+112,y
        sta     x5
        txa
        adc     _gCoeffBuf+113,y
        sta     x5+1

        ;x17 = x5 + x7;
        clc
        lda     x5
        adc     x7
        sta     x17
        lda     x5+1
        adc     x7+1
        sta     x17+1

        sty     tmp3

        ;res1 = imul_b5(x4 - x6)
        sec
        lda     x4
        sbc     x6
        tay
        lda     x4+1
        sbc     x6+1
        tax
        tya
        jsr     _imul_b5
        sta     res1
        stx     res1+1

        ;x24 = res1 - imul_b2(x4
        lda     x4
        ldx     x4+1
        jsr     _imul_b2
        sta     ptr1
        stx     ptr1+1

        sec
        lda     res1
        sbc     ptr1
        sta     x24l+1
        lda     res1+1
        sbc     ptr1+1
        sta     x24h+1

        ;stg26 = imul_b4(x6) - res1;
        ;res2 = stg26 - x17;
        lda     x6
        ldx     x6+1
        jsr     _imul_b4

        sec
        sbc     res1
        tay
        txa
        sbc     res1+1
        tax
        tya

        sec
        sbc     x17
        sta     res2
        txa
        sbc     x17+1
        sta     res2+1

        ;x15 = x5 - x7;
        ;res3 = imul_b1_b3(x15) - res2;
        sec
        lda     x5
        sbc     x7
        pha
        lda     x5+1
        sbc     x7+1
        tax
        pla
        jsr     _imul_b1_b3
        sec
        sbc     res2
        sta     res3
        txa
        sbc     res2+1
        sta     res3+1

        ldy     tmp3
        sec
        lda     _gCoeffBuf,y
        sbc     _gCoeffBuf+64,y
        sta     x31
        lda     _gCoeffBuf+1,y
        tax
        sbc     _gCoeffBuf+65,y
        sta     x31+1

        clc
        lda     _gCoeffBuf,y
        adc     _gCoeffBuf+64,y
        sta     x30
        txa
        adc     _gCoeffBuf+65,y
        sta     x30+1

        sec
        lda     _gCoeffBuf+32,y
        sbc     _gCoeffBuf+96,y
        sta     x12l+1
        lda     _gCoeffBuf+33,y
        tax
        sbc     _gCoeffBuf+97,y
        sta     x12h+1

        clc
        lda     _gCoeffBuf+32,y
        adc     _gCoeffBuf+96,y
        sta     x13
        txa
        adc     _gCoeffBuf+97,y
        sta     x13+1

        sty     tmp3
        ;x32 = imul_b1_b3(x12) - x13;
x12l:
        lda     #$FF
x12h:
        ldx     #$FF
        jsr     _imul_b1_b3
        sec
        sbc     x13
        sta     x32
        txa
        sbc     x13+1
        sta     x32+1

        ;x40 = x30 + x13;
        clc
        lda     x30
        adc     x13
        pha
        lda     x30+1
        adc     x13+1
        tax
        ; t = ((x40 + x17) >> PJPG_DCT_SCALE_BITS) +128;
        ; if (t < 0)
        ;   *pSrc_0_8 = 0;
        ; else if (t & 0xFF00)
        ;    *pSrc_0_8 = 255;
        ; else
        ;   *pSrc_0_8 = (uint8)t;
        pla
        clc
        adc     x17
        pha
        txa
        adc     x17+1
        tax
        pla

        INLINE_ASRAX7

        clc

        adc     #$80
        bcc     :+
        inx

:       cpx     #$00
        beq     clampDone2
        bmi     :+
        lda     #$FF
        bne     clampDone2
:       lda     #$00
clampDone2:
        ldy     tmp3
        sta     _gCoeffBuf,y

        ;x42 = x31 - x32;
        sec
        lda     x31
        sbc     x32
        pha
        lda     x31+1
        sbc     x32+1
        tax

        ; t = ((x42 + res3) >> PJPG_DCT_SCALE_BITS) +128;
        ; if (t < 0)
        ;   *pSrc_2_8 = 0;
        ; else if (t & 0xFF00)
        ;    *pSrc_2_8 = 255;
        ; else
        ;   *pSrc_2_8 = (uint8)t;
        clc
        pla
        adc     res3
        pha
        txa
        adc     res3+1
        tax
        pla
        INLINE_ASRAX7
        clc
        adc     #$80
        bcc     :+
        inx
:       cpx     #$00
        beq     clampDone3
        bmi     :+
        lda     #$FF
        bne     clampDone3
:       lda     #$00
clampDone3:
        sta     _gCoeffBuf+32,y


        ;x43 = x30 - x13;
        sec
        lda     x30
        sbc     x13
        sta     x43l+1
        lda     x30+1
        sbc     x13+1
        sta     x43h+1

        ;x44 = res3 + x24;
        clc
        lda     res3
x24l:
        adc     #$FF
        pha
        lda     res3+1
x24h:
        adc     #$FF
        tax

        ; t = ((x43 + x44) >> PJPG_DCT_SCALE_BITS) +128;
        pla
        clc
x43l:
        adc     #$FF
        pha
        txa
x43h:
        adc     #$FF
        tax
        pla
        INLINE_ASRAX7
        clc
        adc     #$80
        bcc     :+
        inx
:       cpx     #$00
        beq     clampDone4
        bmi     :+
        lda     #$FF
        bne     clampDone4
:       lda     #$00
clampDone4:
        sta     _gCoeffBuf+64,y

        ;x41 = x31 + x32;
        lda     x32
        clc
        adc     x31
        pha
        lda     x32+1
        adc     x31+1
        tax

        ; t = ((x41 - res2) >> PJPG_DCT_SCALE_BITS) +128;
        pla
        sec
        sbc     res2
        pha
        txa
        sbc     res2+1
        tax
        pla
        INLINE_ASRAX7
        clc
        adc     #$80
        bcc     :+
        inx
:       cpx     #$00
        beq     clampDone5
        bmi     :+
        lda     #$FF
        bne     clampDone5
:       lda     #$00
clampDone5:
        sta     _gCoeffBuf+96,y

cont_idct_cols:
        dec     idctCC
        beq     idctColDone
        clc
        iny
        iny
        jmp     nextCol

idctColDone:
        rts

_decodeNextMCU:
        lda     _gRestartInterval
        ora     _gRestartInterval+1
        beq     noRestart

        lda     _gRestartsLeft
        ora     _gRestartsLeft+1
        bne     decRestarts

        jsr     _processRestart
        cmp     #0
        beq     decRestarts
        ldx     #0            ; Return status
        rts

decRestarts:
        lda     _gRestartsLeft
        bne     :+
        dec     _gRestartsLeft+1
:       dec     _gRestartsLeft

noRestart:
        lda     #<_gMCUOrg
        sta     cur_gMCUOrg
        lda     #>_gMCUOrg
        sta     cur_gMCUOrg+1

        lda     #$00
        sta     mcuBlock
nextMcuBlock:
        ; for (mcuBlock = 0; mcuBlock < 2; mcuBlock++) {
        ldy     #$00
        lda     (cur_gMCUOrg),y
        sta     componentID
        tay

        lda     _gCompQuant,y
        beq     :+

        ldx     #<_gQuant1_l
        stx     load_pq0l+1
        stx     load_pq1l+1
        ldx     #>_gQuant1_l
        stx     load_pq0l+2
        stx     load_pq1l+2

        ldx     #<_gQuant1_h
        stx     load_pq0h+1
        stx     load_pq1h+1
        ldx     #>_gQuant1_h
        stx     load_pq0h+2
        stx     load_pq1h+2

        jmp     loadDCTab

:       ldx     #<_gQuant0_l
        stx     load_pq0l+1
        stx     load_pq1l+1
        ldx     #>_gQuant0_l
        stx     load_pq0l+2
        stx     load_pq1l+2

        ldx     #<_gQuant0_h
        stx     load_pq0h+1
        stx     load_pq1h+1
        ldx     #>_gQuant0_h
        stx     load_pq0h+2
        stx     load_pq1h+2

loadDCTab:
        lda     _gCompDCTab,y
        beq     :+

        jsr     _huffDecode1
        jmp     doDec

:       jsr     _huffDecode0

doDec:
        sta     sDMCU

        inc     cur_gMCUOrg
        bne     :+
        inc     cur_gMCUOrg+1

:       and     #$0F
        beq     :+
        jsr     _getBits2
        jmp     doExtend
:       tax

doExtend:
        ; dc = huffExtend(r, s);
        sta     extendX
        stx     extendX+1
        lda     sDMCU
        jsr     _huffExtend
        stx     tmp2
        sta     tmp1

        ; dc = dc + gLastDC[componentID];
        ; gLastDC[componentID] = dc;
        lda     componentID
        asl     a

        tay
        lda     _gLastDC,y
        adc     tmp1
        sta     _gLastDC,y
        tax

        lda     _gLastDC+1,y
        adc     tmp2
        sta     _gLastDC+1,y

        ;gCoeffBuf[0] = dc * pQ[0];
        stx     ptr2
        sta     ptr2+1

load_pq0h:
        lda     $FFFF
        sta     ptr1+1
load_pq0l:
        lda     $FFFF
        sta     ptr1
        jsr     mult16x16x16_direct
        sta     _gCoeffBuf
        stx     _gCoeffBuf+1

        lda     #1
        sta     cur_ZAG_coeff

        ;compACTab = gCompACTab[componentID];
        ldy     componentID
        lda     _gCompACTab,y
        beq     setDec2
setDec3:
        lda     #<_huffDecode3
        ldx     #>_huffDecode3
        jmp     setDec
setDec2:
        lda     #<_huffDecode2
        ldx     #>_huffDecode2
setDec:
        sta     huffDec+1
        stx     huffDec+2

        ;cur_pQ = pQ + 1;
        lda     #1
        sta     cur_pQ

checkZAGLoop:
        lda     cur_ZAG_coeff
        cmp     #64             ; end_ZAG_coeff
        beq     ZAG_Done

doZAGLoop:
huffDec:
        jsr     $FFFF           ; Patched with huffDecode2/3
        sta     sDMCU
        and     #$0F
        beq     :+
        jsr     _getBits2
        jmp     storeExtraBits
:       tax

storeExtraBits:
        sta     extendX
        stx     extendX+1

        lda     sDMCU
        lsr     a
        lsr     a
        lsr     a
        lsr     a
        sta     rDMCU

        lda     sDMCU
        and     #$0F
        sta     sDMCU

        beq     sZero

        lda     rDMCU
        beq     zeroZAGDone
zeroZAG:
        ldx     cur_ZAG_coeff
        ldy     _ZAG_Coeff,x
        lda     #0
        sta     _gCoeffBuf,y
        iny
        sta     _gCoeffBuf,y

        inc     cur_ZAG_coeff

        inc     cur_pQ

        dec     rDMCU
        bne     zeroZAG

zeroZAGDone:
        ;ac = huffExtend(sDMCU
        ; extendX already set
        lda     sDMCU
        jsr     _huffExtend

        ;**cur_ZAG_coeff = ac * *cur_pQ;
        sta     ptr2
        stx     ptr2+1
        tax

        ldy     cur_pQ
load_pq1l:
        lda     $FFFF,y
        sta     ptr1
load_pq1h:
        lda     $FFFF,y
        sta     ptr1+1

        lda     ptr1
        jsr     mult16x16x16_direct
        pha

        ldy     cur_ZAG_coeff
        lda     _ZAG_Coeff,y
        tay
        pla
        sta     _gCoeffBuf,y
        iny
        txa
        sta     _gCoeffBuf,y
        jmp     sNotZero

; Inserted here, in an otherwise unreachable place,
; to be more easily reachable from everywhere we need it
ZAG_Done:
finishZAG:
        ldx     cur_ZAG_coeff
        cpx     #64             ; end_ZAG_coeff
        beq     ZAG_finished

        ldy     _ZAG_Coeff,x
        lda     #0
        sta     _gCoeffBuf,y
        iny
        sta     _gCoeffBuf,y

        inc     cur_ZAG_coeff
        jmp     finishZAG

sZero:
        lda     rDMCU
        cmp     #15
        bne     ZAG_Done

        ; Advance 15
        lda     cur_pQ
        adc     #14           ; 15 with carry set by previous cmp
        sta     cur_pQ

        jmp     checkZAGLoop

sNotZero:
        inc     cur_pQ
        inc     cur_ZAG_coeff
        jmp     checkZAGLoop

ZAG_finished:
        lda     mcuBlock
        jsr     _transformBlock

        inc     mcuBlock
        lda     mcuBlock
        cmp     #2
        bcs     firstMCUBlocksDone
        jmp     nextMcuBlock

firstMCUBlocksDone:
        ; Skip the other blocks, do the minimal work
        lda     mcuBlock
        cmp     _gMaxBlocksPerMCU
        bcs     uselessBlocksDone

nextUselessBlock:
        ldy     #$00
        lda     (cur_gMCUOrg),y
        sta     componentID
        tay

        lda     _gCompDCTab,y
        beq     :+

        jsr     _huffDecode1
        jmp     doDecb

:       jsr     _huffDecode0

doDecb:
        sta     sDMCU

        ldy     componentID
        lda     _gCompACTab,y
        beq     setUDec2
setUDec3:
        lda     #<_huffDecode3
        ldx     #>_huffDecode3
        jmp     setUDec
setUDec2:
        lda     #<_huffDecode2
        ldx     #>_huffDecode2
setUDec:
        sta     uselessDec+1
        stx     uselessDec+2
        lda     sDMCU
        and     #$0F
        beq     :+
        jsr     _getBits2

:       lda     #1
        sta     iDMCU
i64loop:
        ;for (iDMCU = 1; iDMCU != 64; iDMCU++) {
uselessDec:
        jsr     $FFFF   ; Patched with huffDecode2/3

        sta     sDMCU
        and     #$0F
        beq     ZAG2_Done
        pha
        jsr     _getBits2
        pla

        lsr     a
        lsr     a
        lsr     a
        lsr     a
        sec             ; Set carry for for loop inc
        adc     iDMCU
        sta     iDMCU
        cmp     #64
        bne     i64loop

ZAG2_Done:
        inc     mcuBlock
        lda     mcuBlock
        cmp     _gMaxBlocksPerMCU
        bcc     nextUselessBlock

uselessBlocksDone:
        lda     #$00
        tax
        rts

_transformBlock:
        pha

        jsr     _idctRows
        jsr     _idctCols

        pla
        beq     mCZero

        lda     #<(_gMCUBufG+32)
        ldx     #>(_gMCUBufG+32)
        jmp     dstSet
mCZero:
        lda     #<_gMCUBufG
        ldx     #>_gMCUBufG
dstSet:
        sta     copyDest+1
        stx     copyDest+2

        ldy     #31
        ldx     #(31*4)
        sec
tbCopy:
        lda     _gCoeffBuf,x
copyDest:
        sta     $FFFF,y

        txa
        sbc     #4
        tax
        dey
        bpl     tbCopy

        rts

_pjpeg_decode_mcu:
        lda    _gNumMCUSRemainingX
        ora    _gNumMCUSRemainingY
        beq    noMoreBlocks

        jsr    _decodeNextMCU
        bne    retErr

        dec    _gNumMCUSRemainingX
        beq    noMoreX
retOk:
        lda    #0
        ldx    #0
        rts

noMoreX:
        dec    _gNumMCUSRemainingY
        beq    :+
        lda    #((640 + (16 - 1)) >> 4) ; gMaxMCUSPerRow
        sta    _gNumMCUSRemainingX
:       lda    #0
        ldx    #0
        rts

noMoreBlocks:
        lda    #1       ; PJPG_NO_MORE_BLOCKS
retErr:
        ldx    #0
        rts

left: .res 2

_skipVariableMarker:
        lda    #16
        jsr    _getBits1
        cpx    #$00
        bne    :+
        cmp    #$02
        bcc    outErr

        sec
        sbc    #2
        bcs    :+
        dex

:       sta    left
        stx    left+1

contDec:
        lda    #8
        jsr    _getBits1
        lda    left
        bne    :+
        ldx    left+1
        dex
        cpx    #$FF
        beq    outOk
:       dec    left
        bne    contDec

outOk:
        lda    #0
        tax
        rts
outErr:
        lda   #14       ; PJPG_BAD_VARIABLE_MARKER
        ; X already 0
        rts

        .rodata
shit:   .res 32 ; Fix to shit in _decodeNextMCU apparently (_ZAG_Coeff page cross?)

        .bss

;getBit/octet
n:      .res 1
ff:     .res 1
ffcheck:.res 1
final_shift:
        .res 1
ret:    .res 2

;huffExtend
extendX:.res 2

;idctRows
idctRC: .res 1
x7:     .res 2
x5:     .res 2
x15:    .res 2
x17:    .res 2
x6:     .res 2
x4:     .res 2
res1:   .res 2
x24:    .res 2
res2:   .res 2
res3:   .res 2
x30:    .res 2
x31:    .res 2
x13:    .res 2
x32:    .res 2

;idctCols
idctCC: .res 1
x44:    .res 2
x12:    .res 2
x40:    .res 2
x43:    .res 2
x41:    .res 2
x42:    .res 2

;decodeNextMCU
mcuBlock:   .res 1
componentID:.res 1
