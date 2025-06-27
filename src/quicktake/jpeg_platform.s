
        .importzp   tmp1, tmp2, tmp3, ptr1, ptr2, _prev_ram_irq_vector, _prev_rom_irq_vector, c_sp, regbank, ptr4
        .importzp   _zp6sip, _zp8sip, _zp10sip, _zp12sip, _zp6ip, _zp6p
        .import     popptr1
        .import     _extendTests, _extendOffsets, _gBitsLeft, _gBitBuf
        .import     _cache_end, _fillInBuf
        .import     _mul669_l, _mul669_m, _mul669_h
        .import     _mul362_l, _mul362_m, _mul362_h
        .import     _mul277_l, _mul277_m, _mul277_h
        .import     _mul196_l, _mul196_m, _mul196_h
        .import     asrax7
        .import     _gCoeffBuf, _gRestartInterval, _gRestartsLeft
        .import     _gMaxBlocksPerMCU, _processRestart, _gCompACTab, _gCompQuant
        .import     _gQuant0, _gQuant1, _gCompDCTab, _gMCUOrg, _gLastDC, _gCoeffBuf, _ZAG_Coeff
        .import     _gHuffTab0, _gHuffVal0, _gHuffTab1, _gHuffVal1, _gHuffTab2, _gHuffVal2, _gHuffTab3, _gHuffVal3
        .import     _gMCUBufG
        .import     shraxy, decsp4, decsp6, popax, addysp, pushax, tosumulax
        .export     _huffExtend, _getBits1, _getBits2, _getBit
        .export     _imul_b1_b3, _imul_b2, _imul_b4, _imul_b5
        .export     _idctRows, _idctCols, _decodeNextMCU, _transformBlock

.struct hufftable_t
   mMinCode .res 32
   mMaxCode .res 32
   mValPtr  .res 32
.endstruct

_cur_cache_ptr = _prev_ram_irq_vector

; PJPG_INLINE int16 __fastcall__ huffExtend(uint16 x, uint8 sDMCU)

_huffExtend:
        sta     tmp1

        ldy     #0
        ldx     #0
        cmp     #16
        bcs     :+

        asl     a
        tax
        ldy     _extendTests,x
        lda     _extendTests+1,x
        tax

:       cpx     extendX+1
        bcc     retNormal
        bne     retExtend

        cpy     extendX
        bcc     retNormal
        beq     retNormal

retExtend:
        lda     tmp1
        cmp     #16
        bcs     retNormal

        asl     a
        tax
        lda     _extendOffsets,x
        adc     extendX
        tay
        lda     _extendOffsets+1,x
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
        ;sta     _gBitBuf

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
        lda     #$00
        sta     _gBitBuf
        beq     no_lshift2

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
        lda     #$00
        sta     _gBitBuf
        beq     no_lshift4

:       lda     _gBitBuf
:       asl     a
        rol     _gBitBuf+1
        dey
        bne     :-
        sta     _gBitBuf

no_lshift4:
        ldy     tmp1
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
        lda     #$00
        sta     _gBitBuf
        beq     no_lshift5

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

:       lda     n_min_eight,y
        tay
        beq     :++
        lda     ret+1
:       lsr     a
        dey
        bne     :-
        ldx     #0
        rts
:
        lda     ret+1
        ldx     #0
        rts

;uint8 getOctet(uint8 FFCheck)
;warning: param in Y

check_cache_high1:
        lda     _cur_cache_ptr+1
        cmp     _cache_end+1
        bne     continue1
        sty     tmp1
        jsr     _fillInBuf
        ldy     tmp1
        jmp     continue1

check_cache_high2:
        lda     _cur_cache_ptr+1
        cmp     _cache_end+1
        bne     continue3
        sty     tmp1
        jsr     _fillInBuf
        ldy     tmp1
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
        tax                     ; Result in X
        ; Increment buffer pointer
        inc     _cur_cache_ptr
        beq     inc_cache_high1

continue2:
        ; Should we check for $FF?
        bit     ffcheck
        bpl     out
        cpx     #$FF
        bne     out

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
        txa
        rts

; uint8 getBit(void)

_getBit:
        dec     _gBitsLeft
        bmi     :+

        asl     _gBitBuf
        rol     _gBitBuf+1

        lda     #0
        adc     #0
        rts

:
        ldy     #$FF
        jsr     getOctet
        asl     a
        sta     _gBitBuf
        
        lda     #7
        sta     _gBitsLeft
        
        rol     _gBitBuf+1

        lda     #0
        adc     #0
        rts

; uint16 __fastcall__ imul_b1_b3(int16 w)

_imul_b1_b3:
        ldy     #$00
        sty     neg

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
        tya

:
        sta     val
        stx     val+1
        ; dw = mul362_l[l] | mul362_m[l] <<8 | mul362_h[l] <<16;
        ldy    val
        lda    _mul362_l,y
        sta    dw
        ; lda    _mul362_m,y - shortcut right below
        ; sta    dw+1
        lda    _mul362_h,y
        sta    dw+2

        ; dw += (mul362_l[h]) << 8;
        clc
        lda    _mul362_m,y    ; dw+1
        ldy    val+1
        adc    _mul362_l,y
        sta    dw+1

        ; dw += (mul362_m[h]) << 16;
        lda    dw+2
        adc    _mul362_m,y
        sta    dw+2

        ; Was val negative?
        ldy    neg
        bne    :+
        tax
        lda    dw+1
        rts
:
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
        clc
        adc    #1
        bne    :+
        inx
:       rts

; uint16 __fastcall__ imul_b2(int16 w)

_imul_b2:
        ldy     #$00
        sty     neg

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
        tya

:
        sta     val
        stx     val+1
        ; dw = mul669_l[l] | mul669_m[l] <<8 | mul669_h[l] <<16;
        ldy    val
        lda    _mul669_l,y
        sta    dw
        ; lda    _mul669_m,y - shortcut right below
        ; sta    dw+1
        lda    _mul669_h,y
        sta    dw+2

        ; dw += (mul669_l[h]) << 8;
        clc
        lda    _mul669_m,y    ; dw+1
        ldy    val+1
        adc    _mul669_l,y
        sta    dw+1

        ; dw += (mul669_m[h]) << 16;
        lda    dw+2
        adc    _mul669_m,y
        sta    dw+2

        ; Was val negative?
        ldy    neg
        bne    :+
        tax
        lda    dw+1
        rts
:
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
        clc
        adc    #1
        bne    :+
        inx
:       rts

; uint16 __fastcall__ imul_b4(int16 w)

_imul_b4:
        ldy     #$00
        sty     neg

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
        tya

:
        sta     val
        stx     val+1
        ; dw = mul277_l[l] | mul277_m[l] <<8 | mul277_h[l] <<16;
        ldy    val
        lda    _mul277_l,y
        sta    dw
        ; lda    _mul277_m,y - shortcut right below
        ; sta    dw+1
        lda    _mul277_h,y
        sta    dw+2

        ; dw += (mul277_l[h]) << 8;
        clc
        lda    _mul277_m,y    ; dw+1
        ldy    val+1
        adc    _mul277_l,y
        sta    dw+1

        ; dw += (mul277_m[h]) << 16;
        lda    dw+2
        adc    _mul277_m,y
        sta    dw+2

        ; Was val negative?
        ldy    neg
        bne    :+
        tax
        lda    dw+1
        rts
:
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
        clc
        adc    #1
        bne    :+
        inx
:       rts

; uint16 __fastcall__ imul_b5(int16 w)

_imul_b5:
        ldy     #$00
        sty     neg

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
        tya

:
        sta     val
        stx     val+1
        ; dw = mul196_l[l] | mul196_m[l] <<8 | mul196_h[l] <<16;
        ldy    val
        lda    _mul196_l,y
        sta    dw
        ; lda    _mul196_m,y - shortcut right below
        ; sta    dw+1
        lda    _mul196_h,y
        sta    dw+2

        ; dw += (mul196_l[h]) << 8;
        clc
        lda    _mul196_m,y    ; dw+1
        ldy    val+1
        adc    _mul196_l,y
        sta    dw+1

        ; dw += (mul196_m[h]) << 16;
        lda    dw+2
        adc    _mul196_m,y
        sta    dw+2

        ; Was val negative?
        ldy    neg
        bne    :+
        tax
        lda    dw+1
        rts
:
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
        clc
        adc    #1
        bne    :+
        inx
:       rts

; uint8 __fastcall__ huffDecode(const uint8* pHuffVal)
_huffDecode0:
        jsr     _getBit
        sta     code
        lda     #$00
        sta     code+1

        lda     #16
        sta     huffC

        ldy     #$FE
nextLoop0:
        ; *curMaxCode != 0xFFFF?
        iny                     ; FF - 1 - 3 ...
        iny                     ; 0  - 2 - 4 ...
        ldx     _gHuffTab0+1+32,y
        lda     _gHuffTab0+32,y
        cpx     #$FF
        bne     :+
        cmp     #$FF
        beq     noTest0
:       cpx     code+1
        bcc     noTest0          ; high byte <, don't break
        bne     loopDone0        ; high byte >, do break
        cmp     code            ; test low     byte
        bcs     loopDone0        ; low byte >, do break
noTest0:

        asl     code
        rol     code+1
        sty     tmp3
        jsr     _getBit
        ldy     tmp3
        ora     code
        sta     code
        dec     huffC
        bne     nextLoop0
        jmp     huffDecodeDone0
loopDone0:
        clc
        lda     _gHuffTab0+64,y
        adc     code
        sec
        sbc     _gHuffTab0,y
        tay                     ; Backup index

        clc
        lda     _gHuffVal0,y

huffDecodeDone0:
        ldx     #0
        rts

_huffDecode1:
        jsr     _getBit
        sta     code
        lda     #$00
        sta     code+1

        lda     #16
        sta     huffC

        ldy     #$FE
nextLoop1:
        ; *curMaxCode != 0xFFFF?
        iny                     ; FF - 1 - 3 ...
        iny                     ; 0  - 2 - 4 ...
        ldx     _gHuffTab1+1+32,y
        lda     _gHuffTab1+32,y
        cpx     #$FF
        bne     :+
        cmp     #$FF
        beq     noTest1
:       cpx     code+1
        bcc     noTest1          ; high byte <, don't break
        bne     loopDone1        ; high byte >, do break
        cmp     code            ; test low     byte
        bcs     loopDone1        ; low byte >, do break
noTest1:

        asl     code
        rol     code+1
        sty     tmp3
        jsr     _getBit
        ldy     tmp3
        ora     code
        sta     code
        dec     huffC
        bne     nextLoop1
        jmp     huffDecodeDone1
loopDone1:
        clc
        lda     _gHuffTab1+64,y
        adc     code
        sec
        sbc     _gHuffTab1,y
        tay                     ; Backup index

        clc
        lda     _gHuffVal1,y

huffDecodeDone1:
        ldx     #0
        rts

_huffDecode2:
        jsr     _getBit
        sta     code
        lda     #$00
        sta     code+1

        lda     #16
        sta     huffC

        ldy     #$FE
nextLoop2:
        ; *curMaxCode != 0xFFFF?
        iny                     ; FF - 1 - 3 ...
        iny                     ; 0  - 2 - 4 ...
        ldx     _gHuffTab2+1+32,y
        lda     _gHuffTab2+32,y
        cpx     #$FF
        bne     :+
        cmp     #$FF
        beq     noTest2
:       cpx     code+1
        bcc     noTest2          ; high byte <, don't break
        bne     loopDone2        ; high byte >, do break
        cmp     code            ; test low     byte
        bcs     loopDone2        ; low byte >, do break
noTest2:

        asl     code
        rol     code+1
        sty     tmp3
        jsr     _getBit
        ldy     tmp3
        ora     code
        sta     code
        dec     huffC
        bne     nextLoop2
        jmp     huffDecodeDone2
loopDone2:
        clc
        lda     _gHuffTab2+64,y
        adc     code
        sec
        sbc     _gHuffTab2,y
        tay                     ; Backup index

        clc
        lda     _gHuffVal2,y

huffDecodeDone2:
        ldx     #0
        rts

_huffDecode3:
        jsr     _getBit
        sta     code
        lda     #$00
        sta     code+1

        lda     #16
        sta     huffC

        ldy     #$FE
nextLoop3:
        ; *curMaxCode != 0xFFFF?
        iny                     ; FF - 1 - 3 ...
        iny                     ; 0  - 2 - 4 ...
        ldx     _gHuffTab3+1+32,y
        lda     _gHuffTab3+32,y
        cpx     #$FF
        bne     :+
        cmp     #$FF
        beq     noTest3
:       cpx     code+1
        bcc     noTest3          ; high byte <, don't break
        bne     loopDone3        ; high byte >, do break
        cmp     code            ; test low     byte
        bcs     loopDone3        ; low byte >, do break
noTest3:

        asl     code
        rol     code+1
        sty     tmp3
        jsr     _getBit
        ldy     tmp3
        ora     code
        sta     code
        dec     huffC
        bne     nextLoop3
        jmp     huffDecodeDone3
loopDone3:
        clc
        lda     _gHuffTab3+64,y
        adc     code
        sec
        sbc     _gHuffTab3,y
        tay                     ; Backup index

        clc
        lda     _gHuffVal3,y

huffDecodeDone3:
        ldx     #0
        rts


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

        sec
        lda    _gCoeffBuf+4,y
        sbc    _gCoeffBuf+12,y
        pha
        txa
        sbc    _gCoeffBuf+13,y

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

        sec
        lda    x5
        sbc    x7
        sta    x15
        lda    x5+1
        sbc    x7+1
        sta    x15+1

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
        ldy     tmp3
        sta    _gCoeffBuf+4,y
        txa
        sbc    x32+1
        sta    _gCoeffBuf+5,y

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
        clc
        tya
        adc    #16
        tay
        dec    idctRC
        beq    :+
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
        lda     _gCoeffBuf+1,y
        tax
        lda     _gCoeffBuf,y
        jsr     asrax7
        clc
        adc     #$80
        bcc     :+
        inx
        clc
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

        ;x15 = x5 - x7;
        sec
        lda     x5
        sbc     x7
        sta     x15
        lda     x5+1
        sbc     x7+1
        sta     x15+1

        ;x17 = x5 + x7;
        clc
        lda     x5
        adc     x7
        sta     x17
        lda     x5+1
        adc     x7+1
        sta     x17+1

        sty     tmp3

        ;res1 = imul_b5(x4 - x6
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
        clc
        lda     res3
        adc     x24
        sta     x44
        lda     res3+1
        adc     x24+1
        sta     x44+1

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
        sta     x12
        lda     _gCoeffBuf+33,y
        tax
        sbc     _gCoeffBuf+97,y
        sta     x12+1

        clc
        lda     _gCoeffBuf+32,y
        adc     _gCoeffBuf+96,y
        sta     x13
        txa
        adc     _gCoeffBuf+97,y
        sta     x13+1

        sty     tmp3
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
        sec
        lda     x31
        sbc     x32
        sta     x42
        lda     x31+1
        sbc     x32+1
        sta     x42+1

        ;x40 = x30 + x13;
        clc
        lda     x30
        adc     x13
        sta     x40
        lda     x30+1
        adc     x13+1
        sta     x40+1

        ;x43 = x30 - x13;
        sec
        lda     x30
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
        beq     clampDone2
:       cpx     #0
        beq     clampDone2
        lda     #$FF
clampDone2:
        ldy     tmp3
        sta     _gCoeffBuf,y

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
        beq     clampDone3
:       cpx     #0
        beq     clampDone3
        lda     #$FF
clampDone3:
        sta     _gCoeffBuf+32,y

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
        beq     clampDone4
:       cpx     #0
        beq     clampDone4
        lda     #$FF
clampDone4:
        sta     _gCoeffBuf+64,y

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
        beq     clampDone5
:       cpx     #0
        beq     clampDone5
        lda     #$FF
clampDone5:
        sta     _gCoeffBuf+96,y

cont_idct_cols:
        clc
        iny
        iny

        dec     idctCC
        beq     idctColDone
        jmp     nextCol

idctColDone:
        rts

cur_gMCUOrg   = regbank+0
cur_pQ        = regbank+2
cur_ZAG_coeff = regbank+4

_decodeNextMCU:
        jsr     decsp6          ; Backup regbank
        ldy     #5
:       lda     regbank+0,y
        sta     (c_sp),y
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
:       lda     (c_sp),y
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

        lda     #<_gQuant1
        sta     load_pq0+1
        sta     load_pq0b+1
        sta     load_pq1+1
        sta     load_pq2+1
        lda     #>_gQuant1
        sta     load_pq0+2
        sta     load_pq0b+2
        sta     load_pq1+2
        sta     load_pq2+2

        jmp     loadDCTab

:       lda     #<_gQuant0
        sta     load_pq0+1
        sta     load_pq0b+1
        sta     load_pq1+1
        sta     load_pq2+1
        lda     #>_gQuant0
        sta     load_pq0+2
        sta     load_pq0b+2
        sta     load_pq1+2
        sta     load_pq2+2

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
        lda     (ptr1),y
        adc     tmp1
        pha
        iny
        lda     (ptr1),y
        adc     tmp2
        sta     (ptr1),y
        tax
        pla
        dey
        sta     (ptr1),y

        ;gCoeffBuf[0] = dc * pQ[0];
        jsr     pushax
        ldy     #1
load_pq0:
        ldx     $FFFF,y
load_pq0b:
        lda     $FFFF

        jsr     tosumulax
        sta     _gCoeffBuf
        stx     _gCoeffBuf+1

        lda     #1
        sta     cur_ZAG_coeff

        lda     #64
        sta     end_ZAG_coeff

        ;compACTab = gCompACTab[componentID];
        ldy     componentID
        lda     _gCompACTab,y
        sta     compACTab

        ;cur_pQ = pQ + 1;
        lda     #2
        sta     cur_pQ

checkZAGLoop:
        lda     cur_ZAG_coeff
        cmp     end_ZAG_coeff
        bne     doZAGLoop       ; No need to check high byte
        jmp     ZAG_Done

doZAGLoop:
        lda     compACTab
        beq     :+

        jsr     _huffDecode3
        jmp     doDec2

:       jsr     _huffDecode2

doDec2:
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
        lda     #$00
        sta     rDMCU+1

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

        lda     cur_pQ
        clc
        adc     #2
        sta     cur_pQ

        dec     rDMCU
        bne     zeroZAG

zeroZAGDone:
        ;ac = huffExtend(sDMCU
        ; extendX already set
        lda     sDMCU
        jsr     _huffExtend

        ;**cur_ZAG_coeff = ac * *cur_pQ;
        jsr     pushax

        ldy     cur_pQ
load_pq1:
        lda     $FFFF,y
        iny
load_pq2:
        ldx     $FFFF,y
        jsr     tosumulax
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

sZero:
        lda     rDMCU
        cmp     #15
        bne     ZAG_Done

        ; Advance 15
decS:
        lda     cur_ZAG_coeff
        adc     #14             ; Carry set here (we want to add #15)
 
        lda     cur_pQ
        adc     #(15*2)
        sta     cur_pQ

        jmp     checkZAGLoop

sNotZero:
        clc
        lda     cur_pQ
        adc     #2
        sta     cur_pQ

        inc     cur_ZAG_coeff
        jmp     checkZAGLoop

ZAG_Done:
finishZAG:
        lda     cur_ZAG_coeff
        cmp     end_ZAG_coeff
        beq     ZAG_finished  ; No need to check high byte

        ldx     cur_ZAG_coeff
        ldy     _ZAG_Coeff,x
        lda     #0
        sta     _gCoeffBuf,y
        iny
        sta     _gCoeffBuf,y

        inc     cur_ZAG_coeff
        jmp     finishZAG

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
        sta     compACTab

        lda     sDMCU
        and     #$0F
        beq     :+
        jsr     _getBits2
        jmp     doExtend2
:       tax

doExtend2:
        sta     extendX
        stx     extendX+1
        lda     sDMCU
        jsr     _huffExtend

        lda     #1
        sta     iDMCU
i64loop:
        ;for (iDMCU = 1; iDMCU != 64; iDMCU++) {
        lda     compACTab
        beq     :+

        jsr     _huffDecode3
        jmp     doDec2b

:       jsr     _huffDecode2

doDec2b:
        sta     sDMCU
        and     #$0F
        pha
        beq     :+
        jsr     _getBits2
        jmp     storeExtraBits2
:       tax

storeExtraBits2:
        tay                     ; keep AX until...

        lda     sDMCU
        lsr     a
        lsr     a
        lsr     a
        lsr     a
        sta     rDMCU
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
        jmp     sZeroDone2

:       lda     rDMCU
        cmp     #$0F
        bne     ZAG2_Done

        lda     iDMCU
        adc     #14             ; 15 with carry
        sta     iDMCU

sZeroDone2:
        inc     iDMCU
        lda     iDMCU
        cmp     #64
        beq     ZAG2_Done
        jmp     i64loop

ZAG2_Done:
        inc     mcuBlock
        lda     mcuBlock
        cmp     _gMaxBlocksPerMCU
        bcs     uselessBlocksDone
        jmp     nextUselessBlock

uselessBlocksDone:
        ldy     #0              ; Restore regbank
:       lda     (c_sp),y
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
        jmp     dstSet
mCZero:
        lda     #<_gMCUBufG
        ldx     #>_gMCUBufG
dstSet:
        sta     pGDst
        stx     pGDst+1
        lda     #<_gCoeffBuf
        sta     pSrc
        lda     #>_gCoeffBuf
        sta     pSrc+1

        ldy     #$00
tbCopy:
        lda     (pSrc),y
        sta     (pGDst),y

        inc     pSrc          ; pSrc is incremented twice (via Y and base)
        bne     :+            ; because it's 16-bits
        inc     pSrc+1

:       iny
        cpy     #64
        bne     tbCopy

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
rDMCU:      .res 2
sDMCU:      .res 1
iDMCU:      .res 1

end_ZAG_coeff:
            .res 2
