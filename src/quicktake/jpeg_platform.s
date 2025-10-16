
        .importzp   tmp1, tmp2, tmp3, tmp4, ptr1, ptr2, ptr3, ptr4, _prev_ram_irq_vector, _prev_rom_irq_vector, c_sp
        .importzp   _zp2, _zp3, _zp4, _zp5, _zp6, _zp8, _zp7, _zp9, _zp10, _zp11, _zp12, _zp13
        .import     popptr1
        .import     _extendTests_l, _extendTests_h, _extendOffsets_l, _extendOffsets_h
        .import     _fillInBuf, _cache
        .import     _mul145_l, _mul145_m
        .import     _mul217_l, _mul217_m
        .import     _mul51_l, _mul51_m
        .import     _mul106_l, _mul106_m
        .import     _gCoeffBuf, _gRestartInterval, _gRestartsLeft
        .import     _gMaxBlocksPerMCU, _processRestart, _gCompACTab, _gCompQuant
        .import     _gQuant0_l, _gQuant1_l, _gQuant0_h, _gQuant1_h
        .import     _gCompDCTab, _gMCUOrg, _gLastDC, _gCoeffBuf
        .import     _ZAG_Coeff
        .import     _gHuffTab0, _gHuffVal0, _gHuffTab1, _gHuffVal1, _gHuffTab2, _gHuffVal2, _gHuffTab3, _gHuffVal3
        .import     _gNumMCUSRemainingX, _gNumMCUSRemainingY
        .import     _gWinogradQuant
        .import     floppy_motor_on
        .import     right_shift_4

        .import     asreax3, inceaxy, tosmul0ax, push0ax
        .import     shraxy, decsp4, popax, addysp
        .export     _getBitsNoFF, _getBitsFF
        .export     _idctRows, _idctCols, _decodeNextMCU
        .export     _pjpeg_decode_mcu, _setQuant, _setACDCTabs
        .export     _createWinogradQuant0, _createWinogradQuant1
        .export     _initFloppyStarter
        .export     _getByteNoFF, _setFFCheck
        .export     _output0, _output1, _output2, _output3
        .export     _outputIdx

        .include    "../lib/mult8x8x16_macro.inc"
        .include    "../lib/mult16xcommon.inc"
        .include    "../lib/mult16x8x16_macro.inc"
        .include    "../lib/mult16x16x16_macro.inc"

; ZP vars. Mind that qt-conv uses some too
bbHigh         = _zp4       ; byte, used in huffDecode
mcuBlock       = _zp5       ; byte, used in _decodeNextMCU
cur_ZAG_coeff  = _zp6       ; byte, used in _decodeNextMCU
rDMCU          = _zp7       ; byte, used in _decodeNextMCU
inputIdx       = _zp7       ; byte, used in idctRows and idctCols
sDMCU          = _zp8       ; byte, used in _decodeNextMCU
iDMCU          = _zp9       ; byte, used in _decodeNextMCU
winoIdx        = _zp9       ; byte, used in createWinogradQuant
_gBitBuf       = _zp10      ; byte, used everywhere, across bands
_outputIdx     = _zp11      ; word, used everywhere, across bands
_gBitsLeft     = _zp13      ; byte, used everywhere, across bands

NO_FF_CHECK = $60           ; RTS
FF_CHECK_ON = $C9           ; CMP #$nn

CACHE_END = _cache + CACHE_SIZE + 4
.assert <CACHE_END = 0, error

.struct hufftable_t
   mMaxCode_l .res 16
   mMaxCode_h .res 16
   mValPtr    .res 16
   mGetMore   .res 16
.endstruct

_cur_cache_ptr = _prev_ram_irq_vector

.macro SHIFT_7RIGHT_AND_CLAMP
.scope
        rol                     ; XXXXXXLh no_care , H->C
        bcs     @neg
@pos:
        eor     #$80
        bmi     @done           ; if A now $80-$FF X does not change
        lda     #$FF
        jmp     @done
@neg:
        eor     #$80
        bpl     @done
        lda     #$00
@done:
.endscope
.endmacro

.macro SHIFT_XA_7RIGHT_AND_CLAMP
.scope                          ; A        X
        cpx     #$80            ; HXXXXXXL hAAAAAAl,  h->C
        SHIFT_7RIGHT_AND_CLAMP
.endscope
.endmacro

.macro SHIFT_YA_7RIGHT_AND_CLAMP
.scope                          ; A        Y
        cpy     #$80            ; HXXXXXXL hAAAAAAl,  h->C
        SHIFT_7RIGHT_AND_CLAMP
.endscope
.endmacro
; Whether to start floppy motor early (patched to softswitch in the code)
motor_on:
        .res        2,$00

_initFloppyStarter:
        lda     floppy_motor_on         ; Patch motor_on if we use a floppy
        beq     :+
        sta     start_floppy_motor_a+1
        sta     start_floppy_motor_b+1
        lda     #$C0                    ; Firmware access space
        sta     start_floppy_motor_a+2
        sta     start_floppy_motor_b+2
:       rts

; uint8 getBit(void)
; Returns with carry set if bit 1
; Saves A if param given
.macro INLINE_GETBIT SAVE_A
.scope
        dec     _gBitsLeft
        bpl     haveBit
        .if .paramcount = 1
        sta     reloadA+1
        .endif
        jsr     getOctet
        sta     _gBitBuf

        lda     #7
        sta     _gBitsLeft
        .if .paramcount = 1
reloadA:
        lda     #$FF
        .endif
haveBit:
        asl     _gBitBuf    ; Sets carry
.endscope
.endmacro

; uint8 getBit(void)
; Returns with carry set if bit 1
; Lets caller count _gBitsLeft in Y
; Saves A if param given
.macro INLINE_GETBIT_COUNT_Y SAVE_A
.scope
        dey
        bpl     haveBit
        .if .paramcount = 1
        sta     reloadA+1
        .endif
        jsr     getOctet
        sta     _gBitBuf

        ldy     #7
        .if .paramcount = 1
reloadA:
        lda     #$FF
        .endif
haveBit:
        asl     _gBitBuf    ; Sets carry
done:
.endscope
.endmacro

; PJPG_INLINE int16 __fastcall__ huffExtend(uint16 x, uint8 sDMCU)
; x = ptr2/+1, sDMCU = X
; Returns in AX or updates ptr2/+1 according to parameter
.macro HUFFEXTEND UPDATEPTR2
.scope
        cpx     #16
        bcs     retNormal

        lda     ptr2+1
        cmp     _extendTests_h,x

        beq     checkLow
        bcc     retExtend
        .ifblank UPDATEPTR2
        tax
        lda     ptr2
        .endif
        jmp     done

checkLow:
        lda     ptr2
        cmp     _extendTests_l,x
        bcs     retNormalX
retExtend:
        lda     _extendOffsets_l,x    ; Carry clear here
        adc     ptr2
        .ifnblank UPDATEPTR2
        sta     ptr2
        .else
        tay
        .endif
        lda     _extendOffsets_h,x
        adc     ptr2+1
        .ifnblank UPDATEPTR2
        sta     ptr2+1
        .else
        tax
        tya
        .endif
        jmp     done

retNormal:
        .ifblank UPDATEPTR2
        lda     ptr2
        .endif
retNormalX:
        .ifblank UPDATEPTR2
        ldx     ptr2+1
        .endif
done:
.endscope
.endmacro
; #define getBitsNoFF(n) getBits(n, 0)
; #define getBits2(n) getBits(n, 1)

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

.macro GET_BITS_SET_FF_ON
        ldx     #FF_CHECK_ON
        stx     ffcheck
.endmacro

.macro GETBITSDIRECT RETURN_LABEL
.scope
        tax
        ldy     _gBitsLeft
        cpx     #9
        bcs     large_n
small_n:
        lda     #0
:       INLINE_GETBIT_COUNT_Y 1
        rol     a
        dex
        bne     :-
        sty     _gBitsLeft
        ; X is zero
        .ifnblank RETURN_LABEL
        jmp     RETURN_LABEL
        .else
        rts
        .endif

large_n:
        lda     n_min_eight,x ; How much more than 8?
        sta     last_few+1

        lda     #0            ; Init result
        sta     bbHigh        ; Incl. high byte

        ldx     #8            ; Get the first eight
:       INLINE_GETBIT_COUNT_Y 1
        rol     a             ; Don't rol high byte so far
        dex
        bne     :-

last_few:
        ldx     #$FF          ; Get the last (n-8)
:       INLINE_GETBIT_COUNT_Y 1
        rol     a
        rol     bbHigh        ; Do rolhigh byte now
        dex
        bne     :-
        sty     _gBitsLeft

        ldx     bbHigh
        .ifnblank RETURN_LABEL
        jmp     RETURN_LABEL
        .else
        rts
        .endif
.endscope
.endmacro

_getBitsNoFF:
        ldx     #NO_FF_CHECK
        jmp     getBits
_getBitsFF:
        ldx     #FF_CHECK_ON
getBits:
        stx     ffcheck
        GETBITSDIRECT

.macro SKIPBITSDIRECT
.scope
        ldy     _gBitsLeft      ; No need to split short/large here
        tax                     ; As we don't care about the result
:       INLINE_GETBIT_COUNT_Y
        dex
        bne     :-
        sty     _gBitsLeft
        ; X is zero
.endscope
.endmacro

inc_cache_high1:
        inc     _cur_cache_ptr+1
        ldy     _cur_cache_ptr+1
        cpy     #(>CACHE_END)-1
        bcc     noread
start_floppy_motor_a:
        sta     motor_on        ; Start drive motor in advance (patched if on floppy)
        cpy     #>CACHE_END
        bne     noread
        sta     abck1           ; Backup AX before reading
        stx     xbck1
        jsr     _fillInBuf
xbck1 = *+1
        ldx     #$FF            ; Restore AX
abck1 = *+1
        lda     #$FF
noread: ldy     #$00            ; Restore Y after check
        jmp     ffcheck

inc_cache_high2:
        inc     _cur_cache_ptr+1
        ldy     _cur_cache_ptr+1
        cpy     #(>CACHE_END)-1
        bcc     noread2
start_floppy_motor_b:
        sta     motor_on         ; Start drive motor in advance
        cpy     #>CACHE_END
        beq     :+
noread2:
        lda     #$FF            ; Return value (which was #$FF)
        rts

:       stx     xbck2           ; Backup X
        jsr     _fillInBuf
xbck2 = *+1
        ldx     #$FF            ; Restore X (no need for Y there)
        lda     #$FF            ; Return value (which was #$FF)
        rts

dec_cache_high:
        dec     _cur_cache_ptr+1
        jmp     stuff_back

_setFFCheck:
        ldy     #NO_FF_CHECK
        cmp     #0
        beq     :+
        ldy     #FF_CHECK_ON
:       sty     ffcheck
        rts

_getByteNoFF:
        ldy     #NO_FF_CHECK
        sty     ffcheck
        ldx     #0

;uint8 getOctet(uint8 FFCheck)
; Destroys A and Y, saves X
getOctet:
        ; Load char from buffer
        ldy     #$00
        lda     (_cur_cache_ptr),y
        ; Increment buffer pointer
        inc     _cur_cache_ptr
        beq     inc_cache_high1

ffcheck:cmp     #$FF          ; Should we check for $FF? patched with RTS if not
        beq     :+
        rts

:       lda     (_cur_cache_ptr),y
        beq     out           ; is it 0 ?

        ; No, stuff back char
        lda     _cur_cache_ptr
        beq     dec_cache_high
        dec     _cur_cache_ptr

stuff_back:
        ldy     #$00
        lda     #$FF
        sta     (_cur_cache_ptr),y
        rts
out:                          ; We read 0 so increment pointer and return valid FF
        inc     _cur_cache_ptr
        beq     inc_cache_high2

        lda     #$FF          ; Return result
        rts

.macro imul TABL, TABM, lowRes, highRes
.scope
        clc

        ; Check if positive
        bpl     pos
        txa                   ; Negate number
        ; val = -val;
        eor     #$FF
        adc     #1
        sta     revl+1
        tax                   ; Set our pos low byte
        tya                   ; Get high byte
        eor    #$FF
        adc    #0
        tay

        lda    TABM,x
        adc    TABL,y

        ldx    TABM,y
        bcc    :+
        inx
        clc

:       tay     ; backup result low byte
        ; dw ^= 0xffffff, dw++
revl:   lda    TABL
        eor    #$FF
        adc    #1
        tya
        eor    #$FF
        adc    #0
        .ifblank lowRes
        tay
        .else
        sta    lowRes
        .endif
        txa
        eor    #$FF
        adc    #0
        .ifblank highRes
        tax
        tya
        .else
        sta    highRes
        .endif
        jmp    done
pos:
        ; We now have high byte in Y, low in X
        lda    TABM,x
        adc    TABL,y
        .ifnblank lowRes
        sta    lowRes
        .endif
        ldx    TABM,y
        bcc    :+
        inx
        clc
:       .ifnblank highRes
        stx    highRes
        .endif
done:
.endscope
.endmacro

.macro imul_reverse TABL, TABM
.scope
        clc
        ; Check if positive
        bpl     pos
        txa                   ; Negate number
        ; val = -val;
        eor     #$FF
        adc     #1

        tax                   ; Set our pos low byte
        tya                   ; Get high byte
        eor    #$FF
        adc    #0
        tay

        ; We now have high byte in Y, low in X
        lda    TABM,x
        adc    TABL,y

        ldx    TABM,y
        bcc    done
        inx
        clc
        jmp    done
pos:
        ; We now have high byte in Y, low in X
        stx    revl+1
        lda    TABM,x
        adc    TABL,y

        ldx    TABM,y
        bcc    :+
        inx
        clc
:       tay     ; backup low byte
        ; dw ^= 0xffffff, dw++
revl:   lda    TABL
        eor    #$FF
        adc    #1
        tya
        eor    #$FF
        adc    #0
        tay
        txa
        eor    #$FF
        adc    #0
        tax
        tya
done:
.endscope
.endmacro

; uint16 __fastcall__ imul_b1(int16 w)
.macro IMUL_B1
        imul _mul145_l, _mul145_m, ,
.endmacro

; uint16 __fastcall__ imul_b2(int16 w)
.macro IMUL_B2 LB, HB
        imul _mul106_l, _mul106_m, LB, HB
.endmacro

; uint16 __fastcall__ imul_b4(int16 w)
.macro IMUL_B4
        imul_reverse _mul217_l, _mul217_m
.endmacro

; uint16 __fastcall__ imul_b5(int16 w)
.macro IMUL_B5
        imul_reverse _mul51_l, _mul51_m
.endmacro

; uint8 __fastcall__ huffDecode(const uint8* pHuffVal)
.macro huffDecode TABLE, VAL, RETURN
.scope
        INLINE_GETBIT
        lda     #0
        rol             ; Now A = 0 or 1 depending on bit

        ldx     #7
nextLoopS:
        ldy     TABLE+hufftable_t::mGetMore+8,x
        bne     incrementS

        cmp     TABLE+hufftable_t::mMaxCode_l+8,x
        bcc     loopDoneS
incrementS:
        INLINE_GETBIT 1
        rol     a
        dex
        bpl     nextLoopS
        jmp     decodeLong

loopDoneS:
        adc     TABLE+hufftable_t::mValPtr+8,x
        tax                     ; Get index

        lda     VAL,x
        jmp     RETURN

decodeLong:
        ldx     #7
        ldy     #$00
        sty     bbHigh
nextLoopL:
        ldy     TABLE+hufftable_t::mGetMore,x
        bne     incrementL

        ldy     TABLE+hufftable_t::mMaxCode_h,x
        cpy     bbHigh
        beq     :+
        bcc     incrementL
:
        cmp     TABLE+hufftable_t::mMaxCode_l,x
        bcc     loopDoneL
incrementL:
        INLINE_GETBIT 1
        rol     a
        rol     bbHigh
        dex
        bpl     nextLoopL

        lda     #$00          ; if i == 16 return 0
        jmp     RETURN
loopDoneL:
        adc     TABLE+hufftable_t::mValPtr,x
        tax                     ; Get index

        lda     VAL,x
        jmp     RETURN
.endscope
.endmacro

.macro CREATE_WINO TABL, TABH
.scope
        ldy     #63
nextQ:
        sty     winoIdx
        ; x *= gWinogradQuant[i];
        lda     _gWinogradQuant,y
        ldx     #$00
        jsr     push0ax
        ldy     winoIdx
        lda     TABL,y
        ldx     TABH,y
        jsr     tosmul0ax

        ; r = (int16)((x + 4) >> (3));
        ldy     #4
        jsr     inceaxy
        jsr     asreax3

        ldy     winoIdx
        sta     TABL,y
        txa
        sta     TABH,y
        dey
        bpl     nextQ
        rts
.endscope
.endmacro

_createWinogradQuant0:
        CREATE_WINO _gQuant0_l, _gQuant0_h

_createWinogradQuant1:
        CREATE_WINO _gQuant1_l, _gQuant1_h

_huffDecode0:
        huffDecode _gHuffTab0, _gHuffVal0, decodeDC

_huffDecode1:
        huffDecode _gHuffTab1, _gHuffVal1, decodeDC

_huffDecode2:
        huffDecode _gHuffTab2, _gHuffVal2, decodeAC

_huffDecode3:
        huffDecode _gHuffTab3, _gHuffVal3, decodeAC

_huffDecode0_skip:
        huffDecode _gHuffTab0, _gHuffVal0, skipDC

_huffDecode1_skip:
        huffDecode _gHuffTab1, _gHuffVal1, skipDC

_huffDecode2_skip:
        huffDecode _gHuffTab2, _gHuffVal2, skipAC

_huffDecode3_skip:
        huffDecode _gHuffTab3, _gHuffVal3, skipAC

; void idctRows(void

_idctRows:
        ldy    #0
nextIdctRowsLoop:

        lda    _gCoeffBuf+2,y
        bne    full_idct_rows
        lda    _gCoeffBuf+4,y
        bne    full_idct_rows

        lda    _gCoeffBuf+3,y
        bne    full_idct_rows
        lda    _gCoeffBuf+5,y
        bne    full_idct_rows

        ; Short circuit the 1D IDCT if only the DC component is non-zero
        lda    _gCoeffBuf,y
        sta    _gCoeffBuf+2,y
        sta    _gCoeffBuf+4,y
        sta    _gCoeffBuf+6,y

        lda    _gCoeffBuf+1,y
        sta    _gCoeffBuf+3,y
        sta    _gCoeffBuf+5,y
        sta    _gCoeffBuf+7,y

        jmp    cont_idct_rows

full_idct_rows:
        sty     inputIdx

rx5l  = ptr1
rx5h  = ptr1+1
rx13l = ptr2
rx13h = ptr2+1
rx30l = ptr3
rx30h = ptr3+1
rres1l= ptr4
rres1h= ptr4+1

        ; x5
        lda    _gCoeffBuf+2,y
        sta    rx5l
        lda    _gCoeffBuf+3,y
        sta    rx5h

        ; x30
        lda    _gCoeffBuf,y
        sta    rx30l
        lda    _gCoeffBuf+1,y
        sta    rx30h

        ; x13
        clc
        ldx    #0
        lda    _gCoeffBuf+4,y
        sta    rx13l
        adc    rx30l
        bcc    :+
        inx                   ; Remember first carry
        clc
:       adc    rx5l
        sta    _gCoeffBuf,y

        lda    _gCoeffBuf+5,y
        sta    rx13h
        adc    rx30h
        cpx    #1               ; Apply first carry
        adc    rx5h
        sta    _gCoeffBuf+1,y

        ; x32 = imul_b2(x13);
        ldx    rx13l
        ldy    rx13h
        IMUL_B2 rx32l, rx32h

        ; res3 = imul_b1(x5);
        ldx    rx5l
        ldy    rx5h
        IMUL_B1

        ; gCoeffBuf[(idctRC)+1] = res3 + x30 - x32;
        clc
        adc    rx30l
        tay
        txa
        adc    rx30h
        tax

        tya
        sec
rx32l = *+1
        sbc    #$FF

        ldy     inputIdx
        sta    _gCoeffBuf+2,y
        txa
rx32h = *+1
        sbc    #$FF
        sta    _gCoeffBuf+3,y

        ; res1 = imul_b5(x5);
        ldx    rx5l
        ldy    rx5h
        IMUL_B5

        ; gCoeffBuf[(idctRC)+2] = res1 + x30 - x13;
        clc
        adc    rx30l
        tay
        txa
        adc    rx30h
        tax

        sec
        tya
        sbc    rx13l
        ldy    inputIdx
        sta    _gCoeffBuf+4,y
        txa
        sbc    rx13h
        sta    _gCoeffBuf+5,y

        ; res2 = imul_b4(x5);
        ldx    rx5l
        ldy    rx5h
        IMUL_B4

        ; gCoeffBuf[(idctRC)+3] = res2 + x30 + x32;
        clc
        adc    rx30l
        tay
        txa
        adc    rx30h
        tax

        tya
        clc
        adc    rx32l
        ldy    inputIdx
        sta    _gCoeffBuf+6,y
        txa
        adc    rx32h
        sta    _gCoeffBuf+7,y

cont_idct_rows:
        cpy    #(16*2)        ; Loop 3 times
        beq    :+
        clc
        tya
        adc    #16
        tay
        jmp    nextIdctRowsLoop

:       rts

; void idctCols(void

_idctCols:
        ldy     #0
nextCol:
        sty     inputIdx
        ldx     _gCoeffBuf,y

        lda     _gCoeffBuf+16,y
        bne     full_idct_cols
        lda     _gCoeffBuf+32,y
        bne     full_idct_cols

        lda     _gCoeffBuf+17,y
        bne     full_idct_cols
        lda     _gCoeffBuf+33,y
        bne     full_idct_cols

        ; Short circuit the 1D IDCT if only the DC component is non-zero
        lda     _gCoeffBuf+1,y
        SHIFT_XA_7RIGHT_AND_CLAMP

        sta     val0
        sta     val1
        ; val2 is A in cont_idct_cols
        sta     val3
        jmp     cont_idct_cols

full_idct_cols:

cx30l = ptr2
cx30h = ptr2+1
cx5l  = ptr3
cx5h  = ptr3+1
cx12l = ptr4
cx12h = ptr4+1

        ; cx30
        stx     cx30l           ; X already _gCoeffBuf,y
        lda     _gCoeffBuf+1,y
        sta     cx30h

        ; cx5
        lda     _gCoeffBuf+16,y
        sta     cx5l
        lda     _gCoeffBuf+17,y
        sta     cx5h

        ; cx12
        lda     _gCoeffBuf+32,y
        sta     cx12l
        lda    _gCoeffBuf+33,y
        sta     cx12h

        ; val0 = ((x30 + x12 + x5) >> PJPG_DCT_SCALE_BITS) +128;
        clc
        txa                     ; Still cx30l
        ldx     #0
        adc     cx12l
        bcc     :+
        inx
        clc
:       adc     cx5l
        tay

        lda     cx30h
        adc     cx12h
        cpx     #1
        adc     cx5h

        SHIFT_YA_7RIGHT_AND_CLAMP
        sta     val0

        ; cx32 = imul_b2(cx12);
        ldx     cx12l
        ldy     cx12h
        IMUL_B2 cx32l, cx32h

        ;res2 = imul_b4(x5);
        ldx     cx5l
        ldy     cx5h
        IMUL_B4

        ; val3 = ((res2 + x30 + x32) >> PJPG_DCT_SCALE_BITS) +128;
        clc
        ldy     #0
        adc     cx30l
        bcc     :+
        iny
        clc
:
        adc     cx32l
        sta     tmp1

        txa
        adc     cx30h
        cpy     #1
        adc     cx32h
        ldy     tmp1

        SHIFT_YA_7RIGHT_AND_CLAMP
        sta     val3

        ;res3 = imul_b1(x5);
        ldx     cx5l
        ldy     cx5h
        IMUL_B1

        ; val1 = ((cres3 + cx30 - cx32) >> PJPG_DCT_SCALE_BITS) +128;
        clc
        adc     cx30l
        tay
        txa
        adc     cx30h
        tax

        sec
        tya
cx32l = *+1
        sbc     #$FF
        tay
        txa
cx32h = *+1
        sbc     #$FF

        SHIFT_YA_7RIGHT_AND_CLAMP
        sta     val1

        ; res1 = imul_b5(x5)
        ldx     cx5l
        ldy     cx5h
        IMUL_B5

        ; cres1 + cx30 - cx12
        clc
        adc     cx30l
        tay
        txa
        adc     cx30h
        tax

        sec
        tya
        sbc     cx12l
        tay
        txa
        sbc     cx12h

        SHIFT_YA_7RIGHT_AND_CLAMP
        ; val2 = A
cont_idct_cols:
        ldy     _outputIdx
_output2 = *+1
        sta     $FF00,y
val0 = *+1
        lda     #$FF
_output0 = *+1
        sta     $FF00,y
val1 = *+1
        lda     #$FF
_output1 = *+1
        sta     $FF00,y
val3 = *+1
        lda     #$FF
_output3 = *+1
        sta     $FF00,y

        iny
        beq     shiftOutput
storeOutputIdx:
        sty     _outputIdx

        ldy     inputIdx

        cpy     #(3*2)
        beq     idctColDone
        iny
        iny
        jmp     nextCol

shiftOutput:
        inc     _output0+1
        inc     _output1+1
        inc     _output2+1
        inc     _output3+1
        jmp     storeOutputIdx

idctColDone:
        rts

; Patch quantization table in decodeNextMCU once
_setQuant:
        ldy     #0            ; 16-bits mults checker
        sty     tmp1

        cmp     #0
        beq     :+

        ldx     _gQuant1_l    ; Value for [0] won't change, set it directly
        stx     pq0l+1
        ldx     _gQuant1_h
        stx     pq0h+1

        ldx     #<_gQuant1_l  ; Set array address
        stx     load_pq1l+1
        ldx     #>_gQuant1_l
        stx     load_pq1l+2

        ldx     #<_gQuant1_h
        stx     load_pq1h+1
        stx     checkQ+1
        ldx     #>_gQuant1_h
        stx     load_pq1h+2
        stx     checkQ+2
        jmp     check_16bits_quants

:       ldx     _gQuant0_l
        stx     pq0l+1
        ldx     _gQuant0_h
        stx     pq0h+1

        ldx     #<_gQuant0_l
        stx     load_pq1l+1
        ldx     #>_gQuant0_l
        stx     load_pq1l+2

        ldx     #<_gQuant0_h
        stx     load_pq1h+1
        stx     checkQ+1
        ldx     #>_gQuant0_h
        stx     load_pq1h+2
        stx     checkQ+2

check_16bits_quants:
        ldx     _ZAG_Coeff,y  ; Is it a coeff we care about?
        bmi     :+
checkQ:
        ldx     _gQuant1_h,y  ; Is quant factor 16bits?
        beq     :+
        inc     tmp1          ; Notice that
:       iny
        cpy     #64
        bne     check_16bits_quants

patch_16bits_quants:
        ldx     tmp1          ; No 16bits quant factors in coeff we use
        bne     :+            ; Patch mults for 16x8
        ldx     #<mult_zero_coeff_8
        stx     zero_coeff_calc+1
        ldx     #>mult_zero_coeff_8
        stx     zero_coeff_calc+2
        ldx     #<mult_coeff_8
        stx     coeff_calc+1
        ldx     #>mult_coeff_8
        stx     coeff_calc+2
 :      rts

; Patch huff decoders in decodeNextMCU once
_setACDCTabs:
        ; DC tab
        lda     _gCompDCTab
        beq     setDCDec2
setDCDec3:
        lda     #<_huffDecode1
        ldx     #>_huffDecode1
        jmp     setACDec
setDCDec2:
        lda     #<_huffDecode0
        ldx     #>_huffDecode0
setDCDec:
        sta     huffDecDC+1
        stx     huffDecDC+2

        ; AC tab
        lda     _gCompACTab
        beq     setACDec2
setACDec3:
        lda     #<_huffDecode3
        ldx     #>_huffDecode3
        jmp     setACDec
setACDec2:
        lda     #<_huffDecode2
        ldx     #>_huffDecode2
setACDec:
        sta     huffDecAC+1
        stx     huffDecAC+2

        ; Skipped DC tab
        lda     _gCompDCTab+1
        beq     setUDCDec2
setUDCDec3:
        lda     #<_huffDecode1_skip
        ldx     #>_huffDecode1_skip
        jmp     setUDCDec
setUDCDec2:
        lda     #<_huffDecode0_skip
        ldx     #>_huffDecode0_skip
setUDCDec:
        sta     uselessDecDC+1
        stx     uselessDecDC+2

        ; Skipped AC tab
        lda     _gCompACTab+1
        beq     setUACDec2
setUACDec3:
        lda     #<_huffDecode3_skip
        ldx     #>_huffDecode3_skip
        jmp     setUACDec
setUACDec2:
        lda     #<_huffDecode2_skip
        ldx     #>_huffDecode2_skip
setUACDec:
        sta     uselessDecAC+1
        stx     uselessDecAC+2

        rts

mult_zero_coeff_16:
pq0h:
        ldx     #$FF
        MULT_16x16r16 zero_coeff_calc_done
        jmp zero_coeff_calc_done

mult_coeff_16:
load_pq1h:
        ldx     $FFFF,y
        MULT_16x16r16 coeff_calc_done
        jmp coeff_calc_done

mult_zero_coeff_8:
        MULT_16x8r16 zero_coeff_calc_done
        jmp zero_coeff_calc_done

mult_coeff_8:
        MULT_16x8r16 coeff_calc_done
        jmp coeff_calc_done

_decodeNextMCU:
        GET_BITS_SET_FF_ON

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
        ldx     #$00
        stx     mcuBlock

nextMcuBlock:
        ; for (mcuBlock = 0; mcuBlock < 2; mcuBlock++) {
zeroBuf:
        ; Zero indices of gCoeffBuf that we'll use
        ; in idctRows
        lda     #0
        sta     _gCoeffBuf+2  ; 1
        sta     _gCoeffBuf+3
        sta     _gCoeffBuf+4  ; 2
        sta     _gCoeffBuf+5
        sta     _gCoeffBuf+16 ; 8
        sta     _gCoeffBuf+17
        sta     _gCoeffBuf+18 ; 9
        sta     _gCoeffBuf+19
        sta     _gCoeffBuf+20 ; 10
        sta     _gCoeffBuf+21
        sta     _gCoeffBuf+32 ; 16
        sta     _gCoeffBuf+33
        sta     _gCoeffBuf+34 ; 17
        sta     _gCoeffBuf+35
        sta     _gCoeffBuf+36 ; 18
        sta     _gCoeffBuf+37

huffDecDC:
        jmp     $FFFF

decodeDC:

        sta     sDMCU
        and     #$0F          ; numExtraBits
        beq     noBits
        GETBITSDIRECT DCBits  ; r = getBitsFF(numExtraBits);
noBits: tax                   ; otherwise set r to uint16 0 (A is 0)
DCBits:
        sta     ptr2         ; dc = huffExtend(r, s);
        stx     ptr2+1
        ldx     sDMCU
        HUFFEXTEND            ; DC in AX now

        ; dc = dc + gLastDC[componentID=0];
        ; gLastDC[componentID=0] = dc;
        clc
        adc     _gLastDC
        sta     _gLastDC
        sta     ptr2          ; Store dc to ptr2 as it's where mult expects it

        txa
        adc     _gLastDC+1
        sta     _gLastDC+1
        sta     ptr2+1

        ;gCoeffBuf[0] = dc * pQ[0];
pq0l:
        lda     #$FF
zero_coeff_calc:
        jmp     mult_zero_coeff_16
zero_coeff_calc_done:

        sta     _gCoeffBuf      ; and store gCoeffBuf[0]
        stx     _gCoeffBuf+1

        ldy     #1              ; start the ZAG_coeff loop

doZAGLoop:
        sty     cur_ZAG_coeff
huffDecAC:
        jmp     $FFFF           ; Patched with huffDecode2 or 3
decodeAC:
        tax                     ; r = s >> 4, and backup value
        lda     right_shift_4,x
        sta     rDMCU
        ; cur_ZAG_coeff += r;
        clc
        adc     cur_ZAG_coeff
        sta     cur_ZAG_coeff
        tay                     ; to Y

        txa                     ; restore value, numExtraBits = s & 0xF
        and     #$0F

        beq     no_data         ; if (!s)

getData:
        ldx     _ZAG_Coeff,y    ; We only do a part of the matrix
        bmi     skip_coeff
        sta     dataS+1
        stx     ZC              ; Remember Zag coeff
        jmp     getACBits       ; extraBits = getBitsFF(numExtraBits);
gotACBits:
        stx     ptr2+1          ; Store for huffExtend
        sta     ptr2            ; Finish storing for huffExtend

        ;ac = huffExtend(sDMCU)
dataS:
        ldx     #$FF
        HUFFEXTEND 1

        ;gCoeffBuf[cur_ZAG_coeff] = ac * pQ[cur_ZAG_coeff];
        ldy     cur_ZAG_coeff
load_pq1l:
        lda     $FFFF,y
coeff_calc:
        jmp     mult_coeff_16
coeff_calc_done:

ZC = *+1
        ldy     #$FF            ; ZAG_Coeff not cur_ZAG_coeff
        sta     _gCoeffBuf,y
        txa
        sta     _gCoeffBuf+1,y

        ldy     cur_ZAG_coeff
        iny
checkZAGLoop:
        cpy     #64             ; end_ZAG_coeff
        bne     doZAGLoop

ZAG_finished:
        jsr     _idctRows
        jsr     _idctCols

        inc     mcuBlock
        ldx     mcuBlock
        cpx     #2
        bcc     :+
        jmp     nextUselessBlock
:       jmp     nextMcuBlock

no_data:
        lda     rDMCU
        cmp     #15
        bne     ZAG_finished
        cpy     #64
        beq     ZAG_finished
        jmp     doZAGLoop

skip_coeff:
        SKIPBITSDIRECT
        ldy     cur_ZAG_coeff
        iny
        cpy     #64
        beq     ZAG_finished
        jmp     doZAGLoop

getACBits:
        GETBITSDIRECT gotACBits

nextUselessBlock:
        ; Skip the other blocks, do the minimal work
        stx     mcuBlock

uselessDecDC:
        jmp     $FFFF
skipDC:
        and     #$0F
        beq     noSBits
        SKIPBITSDIRECT

noSBits:lda     #1
i64loop:
        ;for (iDMCU = 1; iDMCU != 64; iDMCU++) {
        sta     iDMCU
uselessDecAC:
        jmp     $FFFF   ; Patched with huffDecode2/3
skipAC:
        beq     ZAG2_Done
        and     #$0F
        sta     rsx+1
        SKIPBITSDIRECT
        .assert <right_shift_4 = 0, error ; need alignment for patching trick
rsx:    lda     right_shift_4

        sec             ; Set carry for the loop's inc
        adc     iDMCU
        cmp     #64
        bne     i64loop

ZAG2_Done:
        ldx     mcuBlock
        inx
        cpx     _gMaxBlocksPerMCU
        bcc     nextUselessBlock

uselessBlocksDone:
        lda     #$00
        tax
        rts

_pjpeg_decode_mcu:
        jsr    _decodeNextMCU
        bne    retErr

        dec    _gNumMCUSRemainingX
        beq    noMoreX
retOk:
        lda    #0
        tax
        rts

noMoreX:
        dec    _gNumMCUSRemainingY
        beq    :+
        lda    #((640 + (16 - 1)) >> 4) ; gMaxMCUSPerRow
        sta    _gNumMCUSRemainingX
:       lda    #0
retErr:
        ldx    #0
        rts
