
        .import     popptr1
        .importzp   tmp1, tmp2, ptr1, _prev_ram_irq_vector, _prev_rom_irq_vector, sp, regbank, ptr4
        .importzp   _zp6sip, _zp8sip, _zp10sip, _zp12sip, _zp6ip, _zp6p
        .import     _extendTests, _extendOffsets, _gBitsLeft, _gBitBuf
        .import     _cache_end, _fillInBuf
        .import     _mul669_l, _mul669_m, _mul669_h
        .import     _mul362_l, _mul362_m, _mul362_h
        .import     _mul277_l, _mul277_m, _mul277_h
        .import     _mul196_l, _mul196_m, _mul196_h
        .import     _gCoeffBuf, _gRestartInterval, _gRestartsLeft
        .import     _gMaxBlocksPerMCU, _processRestart, _gCompACTab, _gCompQuant
        .import     _gQuant0, _gQuant1, _gCompDCTab, _gMCUOrg, _gLastDC, _gCoeffBuf, _ZAG_Coeff
        .import     _gHuffTab0, _gHuffVal0, _gHuffTab1, _gHuffVal1, _gHuffTab2, _gHuffVal2, _gHuffTab3, _gHuffVal3
        .import     _gMCUBufG
        .import     shraxy, decsp4, decsp6, popax, addysp, pushax, tosumulax
        .export     _huffExtend, _getBits1, _getBits2, _getBit, _huffDecode
        .export     _imul_b1_b3, _imul_b2, _imul_b4, _imul_b5
        .export     _idctRows, _idctCols, _decodeNextMCU, _transformBlock

.struct hufftable_t
   mMinCode .res 32
   mMaxCode .res 32
   mValPtr  .res 16
.endstruct

_cur_cache_ptr = _prev_ram_irq_vector

; PJPG_INLINE int16 __fastcall__ huffExtend(uint16 x, uint8 sDMCU)

asrax7:
        asl                     ;          AAAAAAA0, h->C
        txa
        rol                     ;          XXXXXXLh, H->C
        ldx     #$00            ; 00000000 XXXXXXLh
        bcc     :+
        dex                     ; 11111111 XXXXXXLh if C
:       rts

_huffExtend:
        sta     tmp1

        ldy     #0
        ldx     #0
        lda     tmp1
        cmp     #16
        bcs     :+

        asl     a
        tax
        lda     _extendTests,x
        tay
        inx
        lda     _extendTests,x
        tax

:       cpx     extendX+1
        beq     :+
        bcs     retExtend
:       cpy     extendX
        bcc     retNormal
        beq     retNormal

retExtend:
        lda     tmp1
        cmp     #16
        bcs     retNormal

        asl     a
        tax
        clc
        lda     _extendOffsets,x
        adc     extendX
        tay
        inx
        lda     _extendOffsets,x
        adc     extendX+1
        tax
        tya
        rts

retNormal:
        lda     extendX
        ldx     extendX+1
        rts

; #define getBits1(n) getBits(n, 0)
; #define getBits2(n) getBits(n, 1)

_getBits1:
        stz     ff
        bra     getBits
_getBits2:
        ldx     #1
        stx     ff
getBits:
        sta     n

        lda     #16
        sec
        sbc     n
        sta     final_shift

        lda     _gBitBuf
        sta     ret
        lda     _gBitBuf+1
        sta     ret+1

        lda     n
        cmp     #9
        bcc     n_lt8

        sec
        sbc     #8
        sta     n

        ldy     _gBitsLeft
        beq     no_lshift

        ; no need to check for << 8, that can't be, as _gBitsLeft maximum is 7
        lda     _gBitBuf

:       asl     a
        rol     _gBitBuf+1
        dey
        bne     :-
        sta     _gBitBuf

no_lshift:
        ldy     ff
        jsr     getOctet
        sta     _gBitBuf
        lda     #8
        sec
        sbc     _gBitsLeft
        tay
        beq     no_lshift2
        cpy     #8
        bne     :+

        lda     _gBitBuf
        sta     _gBitBuf+1
        stz     _gBitBuf
        bra     no_lshift2

:       lda     _gBitBuf
:       asl     a
        rol     _gBitBuf+1
        dey
        bne     :-
        sta     _gBitBuf

no_lshift2:
        lda     _gBitBuf+1
        sta     ret

n_lt8:
        lda     _gBitsLeft
        cmp     n
        bcs     enoughBits

        ldy     _gBitsLeft
        beq     no_lshift3

        ; no need to check for << 8, that can't be
        lda     _gBitBuf
:       asl     a
        rol     _gBitBuf+1
        dey
        bne     :-
        sta     _gBitBuf

no_lshift3:
        ldy     ff
        jsr     getOctet
        ora     _gBitBuf
        sta     _gBitBuf

        lda     n
        sec
        sbc     _gBitsLeft
        sta     tmp1

        tay
        beq     no_lshift4

        cpy     #8
        bne     :+

        lda     _gBitBuf
        sta     _gBitBuf+1
        stz     _gBitBuf
        bra     no_lshift4

:       lda     _gBitBuf
:       asl     a
        rol     _gBitBuf+1
        dey
        bne     :-
        sta     _gBitBuf

no_lshift4:
        lda     #8
        sec
        sbc     tmp1
        sta     _gBitsLeft
        bra     no_lshift5

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
        stz     _gBitBuf
        bra     no_lshift5

:       lda     _gBitBuf
:       asl     a
        rol     _gBitBuf+1
        dey
        bne     :-
        sta     _gBitBuf

no_lshift5:
        lda     ret
        ldx     ret+1
        ldy     final_shift
        jmp     shraxy

;uint8 getOctet(uint8 FFCheck)
;warning: param in Y

getOctet:
        ; Do we need to fill buffer?
        lda     _cur_cache_ptr+1
        cmp     _cache_end+1
        bcc     :+
        lda     _cur_cache_ptr
        cmp     _cache_end
        bcc     :+
        phy
        jsr     _fillInBuf
        ply

:       ; Load char from buffer
        lda     (_cur_cache_ptr)
        tax                     ; Result in X
        ; Increment buffer pointer
        inc     _cur_cache_ptr
        bne     :+
        inc     _cur_cache_ptr+1

:       ; Should we check for $FF?
        cpy     #0
        beq     out
        cpx     #$FF
        bne     out

        ; Yes. Read again.
        lda     _cur_cache_ptr+1
        cmp     _cache_end+1
        bcc     :+
        lda     _cur_cache_ptr
        cmp     _cache_end
        bcc     :+
        jsr     _fillInBuf

:       lda     (_cur_cache_ptr)
        inc     _cur_cache_ptr
        bne     :+
        inc     _cur_cache_ptr+1

:       cmp     #$00
        beq     out

        ; Stuff back chars
        sta     (_cur_cache_ptr)
        lda     _cur_cache_ptr
        bne     :+
        dec     _cur_cache_ptr+1
:       dec     _cur_cache_ptr
        lda     #$FF
        sta     (_cur_cache_ptr)
        lda     _cur_cache_ptr
        bne     :+
        dec     _cur_cache_ptr+1
:       dec     _cur_cache_ptr
out:
        ; Return result
        txa
        ldx     #$00
        rts

; uint8 getBit(void)

_getBit:
        ldx     #0
        lda     _gBitBuf+1
        and     #$80
        beq     :+
        inx

:       dec     _gBitsLeft
        bpl     :+

        phx
        ldy     #1
        jsr     getOctet
        asl     a
        sta     _gBitBuf
        rol     _gBitBuf+1
        plx

        lda     #7
        sta     _gBitsLeft

        txa
        rts

:       asl     _gBitBuf
        rol     _gBitBuf+1

        txa
        rts

; uint16 __fastcall__ imul_b1_b3(int16 w)

_imul_b1_b3:
        stz     neg
        sta     val
        stx     val+1

        cpx     #$80
        bcc     :+
        stx     neg

        ; val = -val;
        clc
        eor     #$FF
        adc     #1
        sta     val
        txa
        eor    #$FF
        adc    #0
        sta    val+1

:       ; dw = mul362_l[l] | mul362_m[l] <<8 | mul362_h[l] <<16;
        ldy    val
        lda    _mul362_l,y
        sta    dw
        lda    _mul362_m,y
        sta    dw+1
        lda    _mul362_h,y
        sta    dw+2

        ; dw += (mul362_l[h]) << 8;
        clc
        ldy    val+1
        lda    dw+1
        adc    _mul362_l,y
        sta    dw+1

        ; dw += (mul362_m[h]) << 16;
        lda    dw+2
        adc    _mul362_m,y
        sta    dw+2

        ; Was val negative?
        lda    neg
        beq    :+

        ; dw ^= 0xffffffff
        lda    #$FF
        eor    dw
        sta    dw
        lda    #$FF
        eor    dw+1
        sta    dw+1
        lda    #$FF
        eor    dw+2
        sta    dw+2

        lda    dw+1
        ldx    dw+2

        ; dw++;
        inc    dw
        bne    :+
        inc    a
        bne    :+
        inx

:       rts

; uint16 __fastcall__ imul_b2(int16 w)

_imul_b2:
        stz    neg
        sta    val
        stx    val+1

        cpx    #$80
        bcc    :+
        stx    neg

        ; val = -val;
        clc
        eor    #$FF
        adc    #1
        sta    val
        txa
        eor    #$FF
        adc    #0
        sta    val+1

:        ; dw = mul669_l[l] | mul669_m[l] <<8 | mul669_h[l] <<16;
        ldy    val
        lda    _mul669_l,y
        sta    dw
        lda    _mul669_m,y
        sta    dw+1
        lda    _mul669_h,y
        sta    dw+2

        ; dw += (mul669_l[h]) << 8;
        clc
        ldy    val+1
        lda    dw+1
        adc    _mul669_l,y
        sta    dw+1

        ; dw += (mul669_m[h]) << 16;
        lda    dw+2
        adc    _mul669_m,y
        sta    dw+2

        ; Was val negative?
        lda    neg
        beq    :+

        ; dw ^= 0xffffffff
        lda    #$FF
        eor    dw
        sta    dw
        lda    #$FF
        eor    dw+1
        sta    dw+1
        lda    #$FF
        eor    dw+2
        sta    dw+2

        lda    dw+1
        ldx    dw+2

        ; dw++;
        inc    dw
        bne    :+
        inc    a
        bne    :+
        inx

:       rts

; uint16 __fastcall__ imul_b4(int16 w)

_imul_b4:
        stz    neg
        sta    val
        stx    val+1

        cpx    #$80
        bcc    :+
        stx    neg

        ; val = -val;
        clc
        eor    #$FF
        adc    #1
        sta    val
        txa
        eor    #$FF
        adc    #0
        sta    val+1

:       ; dw = mul277_l[l] | mul277_m[l] <<8 | mul277_h[l] <<16;
        ldy    val
        lda    _mul277_l,y
        sta    dw
        lda    _mul277_m,y
        sta    dw+1
        lda    _mul277_h,y
        sta    dw+2

        ; dw += (mul277_l[h]) << 8;
        clc
        ldy    val+1
        lda    dw+1
        adc    _mul277_l,y
        sta    dw+1

        ; dw += (mul277_m[h]) << 16;
        lda    dw+2
        adc    _mul277_m,y
        sta    dw+2

        ; Was val negative?
        lda    neg
        beq    :+

        ; dw ^= 0xffffffff
        lda    #$FF
        eor    dw
        sta    dw
        lda    #$FF
        eor    dw+1
        sta    dw+1
        lda    #$FF
        eor    dw+2
        sta    dw+2

        lda    dw+1
        ldx    dw+2

        ; dw++;
        inc    dw
        bne    :+
        inc    a
        bne    :+
        inx

:       rts

; uint16 __fastcall__ imul_b5(int16 w)

_imul_b5:
        stz    neg
        sta    val
        stx    val+1

        cpx    #$80
        bcc    :+
        stx    neg

        ; val = -val;
        clc
        eor    #$FF
        adc    #1
        sta    val
        txa
        eor    #$FF
        adc    #0
        sta    val+1

:        ; dw = mul196_l[l] | mul196_m[l] <<8 | mul196_h[l] <<16;
        ldy    val
        lda    _mul196_l,y
        sta    dw
        lda    _mul196_m,y
        sta    dw+1
        lda    _mul196_h,y
        sta    dw+2

        ; dw += (mul196_l[h]) << 8;
        clc
        ldy    val+1
        lda    dw+1
        adc    _mul196_l,y
        sta    dw+1

        ; dw += (mul196_m[h]) << 16;
        lda    dw+2
        adc    _mul196_m,y
        sta    dw+2

        ; Was val negative?
        lda    neg
        beq    :+

        ; dw ^= 0xffffffff _
        lda    #$FF
        eor    dw
        sta    dw
        lda    #$FF
        eor    dw+1
        sta    dw+1
        lda    #$FF
        eor    dw+2
        sta    dw+2

        lda    dw+1
        ldx    dw+2

        ; dw++;
        inc    dw
        bne    :+
        inc    a
        bne    :+
        inx

:       rts

curMinCode = _zp10sip
curMaxCode = _zp12sip
curValPtr  = _prev_rom_irq_vector

; uint8 __fastcall__ huffDecode(const uint8* pHuffVal)
_huffDecode:
        sta     huffVal
        stx     huffVal+1

        lda     huffTab
        ldx     huffTab+1
        stx     curMinCode+1
        sta     curMinCode

        clc
        adc     #$20
        bcc     :+
        inx
        clc
:       sta     curMaxCode
        stx     curMaxCode+1

        adc     #$20
        bcc     :+
        inx
        clc
:       sta     curValPtr
        stx     curValPtr+1

        jsr     _getBit
        sta     code
        stz     code+1

        lda     #16
        sta     huffC

nextLoop:
        ; huffC == 16 ?
        lda     huffC
        bpl     :+
        lda     #0
        bra     huffDecodeDone

:       ; *curMaxCode != 0xFFFF?
        ldy     #1
        lda     (curMaxCode),y
        tax
        lda     (curMaxCode)
        cpx     #$FF
        bne     :+
        cmp     #$FF
        beq     noTest
:       cpx     code+1
        bcc     noTest          ; high byte <, don't break
        bne     loopDone        ; high byte >, do break
        cmp     code            ; test low     byte
        bcs     loopDone        ; low byte >, do break
noTest:
        dec     huffC
        clc
        lda     curMaxCode
        adc     #2              ; sizeof(uint16)
        sta     curMaxCode
        bcc     :+
        inc     curMaxCode+1
        clc

:       lda     curMinCode
        adc     #2              ; sizeof(uint16)
        sta     curMinCode
        bcc     :+
        inc     curMinCode+1
        clc

:       inc     curValPtr
        bne     :+
        inc     curValPtr+1

:       asl     code
        rol     code+1
        jsr     _getBit
        ora     code
        sta     code
        bra     nextLoop

loopDone:
        clc
        lda     (curValPtr)
        adc     code
        sec
        sbc     (curMinCode)
        tay                     ; Backup index

        clc
        lda     huffVal
        sta     ptr1
        lda     huffVal+1
        sta     ptr1+1
        lda     (ptr1),y

huffDecodeDone:
        ldx     #0
        rts


rowSrc   = regbank+4
rowSrc_1 = regbank+2
rowSrc_2 = regbank+0
rowSrc_3 = _zp6sip
rowSrc_4 = _zp8sip
rowSrc_5 = _zp10sip
rowSrc_6 = _zp12sip
rowSrc_7 = ptr1

; void idctRows(void

_idctRows:
        jsr     decsp6          ; Backup regbank
        ldy     #5
:       lda     regbank+0,y
        sta     (sp),y
        dey
        bpl     :-

        lda     #<(_gCoeffBuf)
        ldx     #>(_gCoeffBuf)
        sta     rowSrc
        stx     rowSrc+1

        lda    #8
        sta    idctRC

nextIdctRowsLoop:
        lda    rowSrc
        ldx    rowSrc+1

        clc
        adc    #2
        bcc    :+
        inx
        clc
:       sta    rowSrc_1
        stx    rowSrc_1+1

        adc    #2
        bcc    :+
        inx
        clc
:       sta    rowSrc_2
        stx    rowSrc_2+1

        adc    #2
        bcc    :+
        inx
        clc
:       sta    rowSrc_3
        stx    rowSrc_3+1

        adc    #2
        bcc    :+
        inx
        clc
:       sta    rowSrc_4
        stx    rowSrc_4+1

        adc    #2
        bcc    :+
        inx
        clc
:       sta    rowSrc_5
        stx    rowSrc_5+1

        adc    #2
        bcc    :+
        inx
        clc
:       sta    rowSrc_6
        stx    rowSrc_6+1

        adc    #2
        bcc    :+
        inx
        clc
:       sta    rowSrc_7
        stx    rowSrc_7+1

        lda    (rowSrc_1)
        bne    full_idct_rows
        lda    (rowSrc_2)
        bne    full_idct_rows
        lda    (rowSrc_3)
        bne    full_idct_rows
        lda    (rowSrc_4)
        bne    full_idct_rows
        lda    (rowSrc_5)
        bne    full_idct_rows
        lda    (rowSrc_6)
        bne    full_idct_rows
        lda    (ptr1)
        bne    full_idct_rows

        ldy    #1
        lda    (rowSrc_1),y
        bne    full_idct_rows
        lda    (rowSrc_2),y
        bne    full_idct_rows
        lda    (rowSrc_3),y
        bne    full_idct_rows
        lda    (rowSrc_4),y
        bne    full_idct_rows
        lda    (rowSrc_5),y
        bne    full_idct_rows
        lda    (rowSrc_6),y
        bne    full_idct_rows
        lda    (ptr1),y
        bne    full_idct_rows

        ; Short circuit the 1D IDCT if only the DC component is non-zero
        lda    (rowSrc)
        sta    (rowSrc_2)
        sta    (rowSrc_4)
        sta    (rowSrc_6)
        ldy    #$01
        lda    (rowSrc),y
        sta    (rowSrc_2),y
        sta    (rowSrc_4),y
        sta    (rowSrc_6),y

        jmp    cont_idct_rows

full_idct_rows:

        ldy    #$01
        clc
        lda    (rowSrc_5)
        adc    (rowSrc_3)
        sta    x7
        lda    (rowSrc_5),y
        tax
        adc    (rowSrc_3),y
        sta    x7+1

        sec
        lda    (rowSrc_5)
        sbc    (rowSrc_3)
        sta    x4
        txa
        sbc    (rowSrc_3),y
        sta    x4+1

        clc
        lda    (rowSrc_1)
        adc    (rowSrc_7)
        sta    x5
        lda    (rowSrc_1),y
        tax
        adc    (rowSrc_7),y
        sta    x5+1

        sec
        lda    (rowSrc_1)
        sbc    (rowSrc_7)
        sta    x6
        txa
        sbc    (rowSrc_7),y
        sta    x6+1

        clc
        lda    (rowSrc)
        adc    (rowSrc_4)
        sta    x30
        lda    (rowSrc),y
        tax
        adc    (rowSrc_4),y
        sta    x30+1

        sec
        lda    (rowSrc)
        sbc    (rowSrc_4)
        sta    x31
        txa
        sbc    (rowSrc_4),y
        sta    x31+1

        clc
        lda    (rowSrc_2)
        adc    (rowSrc_6)
        sta    x13
        lda    (rowSrc_2),y
        tax
        adc    (rowSrc_6),y
        sta    x13+1

        sec
        lda    (rowSrc_2)
        sbc    (rowSrc_6)
        pha
        txa
        sbc    (rowSrc_6),y

        tax
        pla
        jsr    _imul_b1_b3
        sec
        sbc    x13
        sta    x32
        txa
        sbc    x13+1
        sta    x32+1

        lda    x5
        sec
        sbc    x7
        sta    x15
        lda    x5+1
        sbc    x7+1
        sta    x15+1

        lda    x5
        clc
        adc    x7
        sta    x17
        lda    x5+1
        adc    x7+1
        sta    x17+1
        tax

        ;*(rowSrc) = x30 + x13 + x17;
        clc
        adc    x13
        tay
        txa
        adc    x13+1
        tax
        tya
        clc
        adc    x30
        sta    (rowSrc)
        txa
        adc    x30+1
        ldy    #1
        sta    (rowSrc),y

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

        lda    x15
        ldx    x15+1
        jsr    _imul_b1_b3
        sec
        sbc    res2
        sta    res3
        tay
        txa
        sbc    res2+1
        sta    res3+1
        tax

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
        sta    (rowSrc_2)
        txa
        sbc    x32+1
        ldy    #1
        sta    (rowSrc_2),y

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
        sta    (rowSrc_4)
        txa
        sbc    x13+1
        ldy    #1
        sta    (rowSrc_4),y

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
        sta    (rowSrc_6)
        txa
        sbc    res2+1
        ldy    #1
        sta    (rowSrc_6),y

cont_idct_rows:
        clc
        lda    rowSrc
        adc    #16
        sta    rowSrc
        bcc    :+
        inc    rowSrc+1
        clc
:       dec    idctRC
        beq    :+
        jmp    nextIdctRowsLoop

:       ldy     #0              ; Restore regbank
:       lda     (sp),y
        sta     regbank+0,y
        iny
        cpy     #6
        bne     :-
        jmp     addysp

; void idctCols(void

pSrc_0_8 = regbank+0
pSrc_1_8 = _zp12sip
pSrc_2_8 = regbank+2
pSrc_3_8 = regbank+4
pSrc_4_8 = _zp6sip
pSrc_5_8 = _zp10sip
pSrc_6_8 = _zp8sip
pSrc_7_8 = ptr4

_idctCols:
        jsr     decsp6          ; Backup regbank
        ldy     #5
:       lda     regbank+0,y
        sta     (sp),y
        dey
        bpl     :-

        ;init pointers
        lda     #<(_gCoeffBuf)
        sta     pSrc_0_8
        lda     #>(_gCoeffBuf)
        sta     pSrc_0_8+1

        lda     #<(_gCoeffBuf+16)
        sta     pSrc_1_8
        lda     #>(_gCoeffBuf+16)
        sta     pSrc_1_8+1

        lda     #<(_gCoeffBuf+32)
        sta     pSrc_2_8
        lda     #>(_gCoeffBuf+32)
        sta     pSrc_2_8+1

        lda     #<(_gCoeffBuf+48)
        sta     pSrc_3_8
        lda     #>(_gCoeffBuf+48)
        sta     pSrc_3_8+1

        lda     #<(_gCoeffBuf+64)
        sta     pSrc_4_8
        lda     #>(_gCoeffBuf+64)
        sta     pSrc_4_8+1

        lda     #<(_gCoeffBuf+80)
        sta     pSrc_5_8
        lda     #>(_gCoeffBuf+80)
        sta     pSrc_5_8+1

        lda     #<(_gCoeffBuf+96)
        sta     pSrc_6_8
        lda     #>(_gCoeffBuf+96)
        sta     pSrc_6_8+1

        lda     #<(_gCoeffBuf+112)
        sta     pSrc_7_8
        lda     #>(_gCoeffBuf+112)
        sta     pSrc_7_8+1

        lda     #8
        sta     idctCC

nextCol:
        ldy     #1
        lda     (pSrc_2_8)
        bne     full_idct_cols
        lda     (pSrc_4_8)
        bne     full_idct_cols
        lda     (pSrc_6_8)
        bne     full_idct_cols
        lda     (pSrc_2_8),y
        bne     full_idct_cols
        lda     (pSrc_4_8),y
        bne     full_idct_cols
        lda     (pSrc_6_8),y
        bne     full_idct_cols

        ; Short circuit the 1D IDCT if only the DC component is non-zero
        ldy     #1
        lda     (pSrc_0_8),y
        tax
        lda     (pSrc_0_8)
        jsr     asrax7
        clc
        adc     #$80
        bcc     :+
        inx
        clc
:       cpx     #$80
        bcc     :+
        lda     #0
        bra     clampDone1
:       cpx     #0
        beq     clampDone1
        lda     #$FF

clampDone1:
        sta     (pSrc_0_8)
        sta     (pSrc_2_8)
        sta     (pSrc_4_8)
        sta     (pSrc_6_8)
        ldy     #$01
        lda     #$00
        sta     (pSrc_0_8),y
        sta     (pSrc_2_8),y
        sta     (pSrc_4_8),y
        sta     (pSrc_6_8),y
        jmp     cont_idct_cols

full_idct_cols:
        ldy     #1
        sec
        lda     (pSrc_5_8)
        sbc     (pSrc_3_8)
        sta     x4
        lda     (pSrc_5_8),y
        tax
        sbc     (pSrc_3_8),y
        sta     x4+1

        clc
        lda     (pSrc_5_8)
        adc     (pSrc_3_8)
        sta     x7
        txa
        adc     (pSrc_3_8),y
        sta     x7+1

        sec
        lda     (pSrc_1_8)
        sbc     (pSrc_7_8)
        sta     x6
        lda     (pSrc_1_8),y
        tax
        sbc     (pSrc_7_8),y
        sta     x6+1

        clc
        lda     (pSrc_1_8)
        adc     (pSrc_7_8)
        sta     x5
        txa
        adc     (pSrc_7_8),y
        sta     x5+1

        ;x15 = x5 - x7;
        lda     x5
        sec
        sbc     x7
        sta     x15
        lda     x5+1
        sbc     x7+1
        sta     x15+1

        ;x17 = x5 + x7;
        lda     x5
        clc
        adc     x7
        sta     x17
        lda     x5+1
        adc     x7+1
        sta     x17+1

        ;res1 = imul_b5(x4 - x6
        lda     x4
        sec
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
        lda     res1
        sec
        sbc     ptr1
        sta     x24
        lda     res1+1
        sbc     ptr1+1
        sta     x24+1

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

        ;res3 = imul_b1_b3(x15) - res2;
        lda     x15
        ldx     x15+1
        jsr     _imul_b1_b3
        sec
        sbc     res2
        sta     res3
        txa
        sbc     res2+1
        sta     res3+1

        ;x44 = res3 + x24;
        lda     res3
        clc
        adc     x24
        sta     x44
        lda     res3+1
        adc     x24+1
        sta     x44+1

        ldy     #1
        sec
        lda     (pSrc_0_8)
        sbc     (pSrc_4_8)
        sta     x31
        lda     (pSrc_0_8),y
        tax
        sbc     (pSrc_4_8),y
        sta     x31+1

        clc
        lda     (pSrc_0_8)
        adc     (pSrc_4_8)
        sta     x30
        txa
        adc     (pSrc_4_8),y
        sta     x30+1

        sec
        lda     (pSrc_2_8)
        sbc     (pSrc_6_8)
        sta     x12
        lda     (pSrc_2_8),y
        tax
        sbc     (pSrc_6_8),y
        sta     x12+1

        clc
        lda     (pSrc_2_8)
        adc     (pSrc_6_8)
        sta     x13
        txa
        adc     (pSrc_6_8),y
        sta     x13+1

        ;x32 = imul_b1_b3(x12) - x13;
        lda     x12
        ldx     x12+1
        jsr     _imul_b1_b3
        sec
        sbc     x13
        sta     x32
        txa
        sbc     x13+1
        sta     x32+1

        ;x41 = x31 + x32;
        tax
        lda     x32
        clc
        adc     x31
        sta     x41
        txa
        adc     x31+1
        sta     x41+1

        ;x42 = x31 - x32;
        lda     x31
        sec
        sbc     x32
        sta     x42
        lda     x31+1
        sbc     x32+1
        sta     x42+1

        ;x40 = x30 + x13;
        lda     x30
        clc
        adc     x13
        sta     x40
        lda     x30+1
        adc     x13+1
        sta     x40+1

        ;x43 = x30 - x13;
        lda     x30
        sec
        sbc     x13
        sta     x43
        lda     x30+1
        sbc     x13+1
        sta     x43+1

        ; t = ((x40 + x17) >> PJPG_DCT_SCALE_BITS) +128;
        ; if (t < 0)
        ;   *pSrc_0_8 = 0;
        ; else if (t & 0xFF00)
        ;    *pSrc_0_8 = 255;
        ; else
        ;   *pSrc_0_8 = (uint8)t;
        lda     x40
        clc
        adc     x17
        pha
        lda     x40+1
        adc     x17+1
        tax
        pla
        jsr     asrax7
        clc
        adc     #$80
        bcc     :+
        inx
        clc
:       cpx     #$80
        bcc     :+
        lda     #0
        bra     clampDone2
:       cpx     #0
        beq     clampDone2
        lda     #$FF
clampDone2:
        sta     (pSrc_0_8)

        ; t = ((x42 + res3) >> PJPG_DCT_SCALE_BITS) +128;
        ; if (t < 0)
        ;   *pSrc_2_8 = 0;
        ; else if (t & 0xFF00)
        ;    *pSrc_2_8 = 255;
        ; else
        ;   *pSrc_2_8 = (uint8)t;
        lda     x42
        clc
        adc     res3
        pha
        lda     x42+1
        adc     res3+1
        tax
        pla
        jsr     asrax7
        clc
        adc     #$80
        bcc     :+
        inx
        clc
:       cpx     #$80
        bcc     :+
        lda     #0
        bra     clampDone3
:       cpx     #0
        beq     clampDone3
        lda     #$FF
clampDone3:
        sta     (pSrc_2_8)

        lda     x43
        clc
        adc     x44
        pha
        lda     x43+1
        adc     x44+1
        tax
        pla
        jsr     asrax7
        clc
        adc     #$80
        bcc     :+
        inx
        clc
:       cpx     #$80
        bcc     :+
        lda     #0
        bra     clampDone4
:       cpx     #0
        beq     clampDone4
        lda     #$FF
clampDone4:
        sta     (pSrc_4_8)

        lda     x41
        sec
        sbc     res2
        pha
        lda     x41+1
        sbc     res2+1
        tax
        pla
        jsr     asrax7
        clc
        adc     #$80
        bcc     :+
        inx
        clc
:       cpx     #$80
        bcc     :+
        lda     #0
        bra     clampDone5
:       cpx     #0
        beq     clampDone5
        lda     #$FF
clampDone5:
        sta     (pSrc_6_8)

cont_idct_cols:
        clc
        lda     pSrc_0_8
        adc     #2
        sta     pSrc_0_8
        bcc     :+
        inc     pSrc_0_8+1
        clc

:       lda     pSrc_1_8
        adc     #2
        sta     pSrc_1_8
        bcc     :+
        inc     pSrc_1_8+1
        clc

:       lda     pSrc_2_8
        adc     #2
        sta     pSrc_2_8
        bcc     :+
        inc     pSrc_2_8+1
        clc

:       lda     pSrc_3_8
        adc     #2
        sta     pSrc_3_8
        bcc     :+
        inc     pSrc_3_8+1
        clc

:       lda     pSrc_4_8
        adc     #2
        sta     pSrc_4_8
        bcc     :+
        inc     pSrc_4_8+1
        clc

:       lda     pSrc_5_8
        adc     #2
        sta     pSrc_5_8
        bcc     :+
        inc     pSrc_5_8+1
        clc

:       lda     pSrc_6_8
        adc     #2
        sta     pSrc_6_8
        bcc     :+
        inc     pSrc_6_8+1
        clc

:       lda     pSrc_7_8
        adc     #2
        sta     pSrc_7_8
        bcc     :+
        inc     pSrc_7_8+1
        clc

:       dec     idctCC
        beq     idctColDone
        jmp     nextCol

idctColDone:
        ldy     #0              ; Restore regbank
:       lda     (sp),y
        sta     regbank+0,y
        iny
        cpy     #6
        bne     :-
        jmp     addysp

cur_gMCUOrg   = regbank+0
cur_pQ        = regbank+2
cur_ZAG_coeff = regbank+4

_decodeNextMCU:
        jsr     decsp6          ; Backup regbank
        ldy     #5
:       lda     regbank+0,y
        sta     (sp),y
        dey
        bpl     :-

        lda     _gRestartInterval
        ora     _gRestartInterval+1
        beq     noRestart

        lda     _gRestartsLeft
        ora     _gRestartsLeft+1
        bne     decRestarts

        jsr     _processRestart
        cmp     #0
        beq     decRestarts
        pha

        ldy     #0              ; Restore regbank
:       lda     (sp),y
        sta     regbank+0,y
        iny
        cpy     #6
        bne     :-

        pla
        ldx     #0
        jmp     addysp

decRestarts:
        lda     _gRestartsLeft
        bne     :+
        dec     _gRestartsLeft+1
:       dec     _gRestartsLeft

noRestart:
        lda     #<(_gMCUOrg)
        sta     cur_gMCUOrg
        lda     #>(_gMCUOrg)
        sta     cur_gMCUOrg+1

        stz     mcuBlock
nextMcuBlock:
        ; for (mcuBlock = 0; mcuBlock < 2; mcuBlock++) {
        lda     (cur_gMCUOrg)
        sta     componentID
        tay

        lda     _gCompQuant,y
        beq     :+

        lda     #<(_gQuant1)
        sta     pQ
        lda     #>(_gQuant1)
        sta     pQ+1

        bra     loadDCTab

:       lda     #<(_gQuant0)
        sta     pQ
        lda     #>(_gQuant0)
        sta     pQ+1

loadDCTab:
        lda     _gCompDCTab,y
        beq     :+

        lda     #<(_gHuffTab1)
        ldx     #>(_gHuffTab1)
        sta     huffTab         ; Use global var instead of func param, for speed
        stx     huffTab+1
        lda     #<(_gHuffVal1)
        ldx     #>(_gHuffVal1)
        bra     doDec

:       lda     #<(_gHuffTab0)
        ldx     #>(_gHuffTab0)
        sta     huffTab
        stx     huffTab+1
        lda     #<(_gHuffVal0)
        ldx     #>(_gHuffVal0)

doDec:
        jsr     _huffDecode
        sta     sDMCU

        inc     cur_gMCUOrg
        bne     :+
        inc     cur_gMCUOrg+1

:       lda     sDMCU
        and     #$0F
        beq     :+
        jsr     _getBits2
        bra     doExtend
:       tax

doExtend:
        sta     extendX
        stx     extendX+1
        lda     sDMCU
        jsr     _huffExtend
        stx     tmp2
        sta     tmp1

        ldx     #0
        lda     componentID
        asl     a
        bcc     :+
        inx
        clc

:       adc     #<_gLastDC
        sta     ptr1
        txa
        adc     #>_gLastDC
        sta     ptr1+1

        clc
        ldy     #0
        lda     (ptr1)
        adc     tmp1
        pha
        iny
        lda     (ptr1),y
        adc     tmp2
        sta     (ptr1),y
        tax
        pla
        sta     (ptr1)

        ;gCoeffBuf[0] = dc * pQ[0];
        jsr     pushax
        lda     pQ
        sta     ptr1
        lda     pQ+1
        sta     ptr1+1
        ldy     #1
        lda     (ptr1),y
        tax
        lda     (ptr1)
        jsr     tosumulax
        sta     _gCoeffBuf
        stx     _gCoeffBuf+1

        lda     #<(_ZAG_Coeff+2)  ; 1*sizeof(uint16)
        sta     cur_ZAG_coeff
        lda     #>(_ZAG_Coeff+2)
        sta     cur_ZAG_coeff+1

        lda     #<(_ZAG_Coeff+126) ; (64-1)*sizeof(uint16)
        sta     end_ZAG_coeff
        lda     #>(_ZAG_Coeff+126)
        sta     end_ZAG_coeff+1

        ;compACTab = gCompACTab[componentID];
        ldy     componentID
        lda     _gCompACTab,y
        sta     compACTab

        ;cur_pQ = pQ + 1;
        lda     pQ
        clc
        adc     #2
        sta     cur_pQ
        lda     pQ+1
        adc     #0
        sta     cur_pQ+1

        lda     cur_ZAG_coeff
checkZAGLoop:
        cmp     end_ZAG_coeff
        bne     doZAGLoop

        lda     cur_ZAG_coeff+1
        cmp     end_ZAG_coeff+1
        bne     doZAGLoop
        jmp     ZAG_Done

doZAGLoop:
        lda     compACTab
        beq     :+

        lda     #<(_gHuffTab3)
        ldx     #>(_gHuffTab3)
        sta     huffTab
        stx     huffTab+1
        lda     #<(_gHuffVal3)
        ldx     #>(_gHuffVal3)
        bra     doDec2

:       lda     #<(_gHuffTab2)
        ldx     #>(_gHuffTab2)
        sta     huffTab
        stx     huffTab+1
        lda     #<(_gHuffVal2)
        ldx     #>(_gHuffVal2)

doDec2:
        jsr     _huffDecode
        sta     sDMCU
        and     #$0F
        beq     :+
        jsr     _getBits2
        bra     storeExtraBits
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
        stz     rDMCU+1
        lda     sDMCU
        and     #$0F
        sta     sDMCU

        lda     sDMCU
        beq     sZero

        lda     rDMCU
        ora     rDMCU+1
        beq     zeroZAGDone
zeroZAG:
        lda     (cur_ZAG_coeff)
        sta     ptr1
        ldy     #1
        lda     (cur_ZAG_coeff),y
        sta     ptr1+1
        lda     #0
        sta     (ptr1)
        sta     (ptr1),y

        clc
        lda     cur_ZAG_coeff
        adc     #2
        sta     cur_ZAG_coeff
        bcc     :+
        inc     cur_ZAG_coeff+1
        clc

:       lda     cur_pQ
        adc     #2
        sta     cur_pQ
        bcc     :+
        inc     cur_pQ+1
        clc

:       lda     rDMCU
        bne     :+
        dec     rDMCU+1
        bmi     zeroZAGDone
:       dec     rDMCU
        bne     zeroZAG

zeroZAGDone:
        ;ac = huffExtend(sDMCU
        ; extendX already set
        lda     sDMCU
        jsr     _huffExtend

        ;*cur_ZAG_coeff = ac * *cur_pQ;
        jsr     pushax
        ldy     #1
        lda     (cur_pQ),y
        tax
        lda     (cur_pQ)
        jsr     tosumulax
        pha
        lda     (cur_ZAG_coeff)
        sta     ptr1
        ldy     #1
        lda     (cur_ZAG_coeff),y
        sta     ptr1+1
        pla
        sta     (ptr1)
        txa
        sta     (ptr1),y
        bra     sNotZero

sZero:
        lda     rDMCU+1
        bne     ZAG_Done
        lda     rDMCU
        cmp     #$0F
        bne     ZAG_Done

        sta     sDMCU
decS:
        lda     (cur_ZAG_coeff)
        sta     ptr1
        ldy     #1
        lda     (cur_ZAG_coeff),y
        sta     ptr1+1
        lda     #0
        sta     (ptr1)
        sta     (ptr1),y

        clc
        lda     cur_ZAG_coeff
        adc     #2
        sta     cur_ZAG_coeff
        bcc     :+
        inc     cur_ZAG_coeff+1
        clc

:       lda     cur_pQ
        adc     #2
        sta     cur_pQ
        bcc     :+
        inc     cur_pQ+1
        clc
:       dec     sDMCU
        bne     decS

sNotZero:
        clc
        lda     cur_pQ
        adc     #2
        sta     cur_pQ
        bcc     :+
        inc     cur_pQ+1
        clc

:       lda     cur_ZAG_coeff
        adc     #2
        sta     cur_ZAG_coeff
        bcc     :+
        inc     cur_ZAG_coeff+1
:       jmp     checkZAGLoop

ZAG_Done:
        lda     cur_ZAG_coeff
finishZAG:
        cmp     end_ZAG_coeff
        bne     :+
        lda     cur_ZAG_coeff+1
        cmp     end_ZAG_coeff+1
        beq     ZAG_finished

:       lda     (cur_ZAG_coeff)
        sta     ptr1
        ldy     #1
        lda     (cur_ZAG_coeff),y
        sta     ptr1+1
        lda     #0
        sta     (ptr1)
        sta     (ptr1),y

        clc
        lda     cur_pQ
        adc     #2
        sta     cur_pQ
        bcc     :+
        inc     cur_pQ+1
        clc

:       lda     cur_ZAG_coeff
        adc     #2
        sta     cur_ZAG_coeff
        bcc     finishZAG
        inc     cur_ZAG_coeff+1
        clc
        bra     finishZAG

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
        bcc     nextUselessBlock
        jmp     uselessBlocksDone

nextUselessBlock:
        lda     (cur_gMCUOrg)
        sta     componentID
        tay

        lda     _gCompDCTab,y
        beq     :+

        lda     #<(_gHuffTab1)
        ldx     #>(_gHuffTab1)
        sta     huffTab
        stx     huffTab+1
        lda     #<(_gHuffVal1)
        ldx     #>(_gHuffVal1)
        bra     doDecb

:       lda     #<(_gHuffTab0)
        ldx     #>(_gHuffTab0)
        sta     huffTab
        stx     huffTab+1
        lda     #<(_gHuffVal0)
        ldx     #>(_gHuffVal0)

doDecb:
        jsr     _huffDecode
        and     #$0F
        sta     sDMCU

        ldy     componentID
        lda     _gCompACTab,y
        sta     compACTab

        lda     sDMCU
        beq     :+
        jsr     _getBits2
        bra     doExtend2
:       tax

doExtend2:
        sta     extendX
        stx     extendX+1
        lda     sDMCU
        jsr     _huffExtend

        lda     #1
        sta     iDMCU
i63loop:
        ;for (iDMCU = 1; iDMCU != 64; iDMCU++) {
        lda     compACTab
        beq     :+

        lda     #<(_gHuffTab3)
        ldx     #>(_gHuffTab3)
        sta     huffTab
        stx     huffTab+1
        lda     #<(_gHuffVal3)
        ldx     #>(_gHuffVal3)
        bra     doDec2b

:       lda     #<(_gHuffTab2)
        ldx     #>(_gHuffTab2)
        sta     huffTab
        stx     huffTab+1
        lda     #<(_gHuffVal2)
        ldx     #>(_gHuffVal2)

doDec2b:
        jsr     _huffDecode
        sta     sDMCU
        and     #$0F
        pha
        beq     :+
        jsr     _getBits2
        bra     storeExtraBits2
:       tax

storeExtraBits2:
        tay                     ; keep AX until...

        lda     sDMCU
        lsr     a
        lsr     a
        lsr     a
        lsr     a
        sta     rDMCU
        stz     rDMCU+1
        pla
        sta     sDMCU
        beq     :+

        clc
        lda     iDMCU
        adc     rDMCU
        sta     iDMCU
        tya                     ; there
        sta     extendX
        stx     extendX+1
        lda     sDMCU
        jsr     _huffExtend
        bra     sZeroDone2

:       lda     rDMCU
        cmp     #$0F
        bne     ZAG2_Done
        lda     rDMCU+1
        bne     ZAG2_Done

        lda     iDMCU
        clc
        adc     #$0F
        sta     iDMCU

sZeroDone2:
        inc     iDMCU
        lda     iDMCU
        cmp     #64
        beq     ZAG2_Done
        jmp     i63loop

ZAG2_Done:
        inc     mcuBlock
        lda     mcuBlock
        cmp     _gMaxBlocksPerMCU
        bcs     uselessBlocksDone
        jmp     nextUselessBlock

uselessBlocksDone:
        ldy     #0              ; Restore regbank
:       lda     (sp),y
        sta     regbank+0,y
        iny
        cpy     #6
        bne     :-
        lda     #$00
        tax
        jmp     addysp

pGDst = _zp6p
pSrc  = _zp8sip

_transformBlock:
        pha

        jsr     _idctRows
        jsr     _idctCols

        pla
        beq     mCZero

        lda     #<(_gMCUBufG+64)
        ldx     #>(_gMCUBufG+64)
        bra     dstSet
mCZero:
        lda     #<(_gMCUBufG)
        ldx     #>(_gMCUBufG)
dstSet:
        sta     pGDst
        stx     pGDst+1
        lda     #<(_gCoeffBuf)
        sta     pSrc
        lda     #>(_gCoeffBuf)
        sta     pSrc+1

        ldy     #64
        clc

tbCopy:
        lda     (pSrc)
        sta     (pGDst)

        inc     pGDst
        bne     :+
        inc     pGDst+1

:       lda     pSrc
        adc     #2
        sta     pSrc
        bcc     :+
        inc     pSrc+1
        clc

:       dey
        bne     tbCopy

        rts

        .bss

;getBit/octet
n:      .res 1
ff:     .res 1
final_shift:
        .res 1
ret:    .res 2

;imul
dw:     .res 4
val:    .res 2
neg:    .res 1

;huffDecode
huffTab:.res 2
huffVal:.res 2
huffC:  .res 1
code:   .res 2

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
status:     .res 1
mcuBlock:   .res 1
componentID:.res 1
compACTab:  .res 1
pQ:         .res 2
rDMCU:      .res 2
sDMCU:      .res 1
iDMCU:      .res 1

end_ZAG_coeff:
            .res 2
