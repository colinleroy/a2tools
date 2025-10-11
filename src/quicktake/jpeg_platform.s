
        .importzp   tmp1, tmp2, tmp3, tmp4, ptr1, ptr2, ptr3, ptr4, _prev_ram_irq_vector, _prev_rom_irq_vector, c_sp
        .importzp   _zp2, _zp3, _zp4, _zp5, _zp6, _zp8, _zp7, _zp9, _zp10, _zp11, _zp12, _zp13
        .import     popptr1
        .import     _extendTests_l, _extendTests_h, _extendOffsets_l, _extendOffsets_h
        .import     _fillInBuf, _cache
        .import     _mul362_l, _mul362_m, _mul362_h
        .import     _mul473_l, _mul473_m, _mul473_h
        .import     _mul196_l, _mul196_m
        .import     _gCoeffBuf, _gRestartInterval, _gRestartsLeft
        .import     _gMaxBlocksPerMCU, _processRestart, _gCompACTab, _gCompQuant
        .import     _gQuant0_l, _gQuant1_l, _gQuant0_h, _gQuant1_h
        .import     _gCompDCTab, _gMCUOrg, _gLastDC_l, _gLastDC_h, _gCoeffBuf
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
_gBitBuf       = _zp2       ; byte, used everywhere
_outputIdx     = _zp4       ; byte, used everywhere
bbHigh         = _zp5       ; byte, used in huffDecode
; zp6-7 USED in qt-conv so only use it temporarily
mcuBlock       = _zp7       ; byte, used in _decodeNextMCU

_gBitsLeft     = _zp8       ; byte, used everywhere
cur_ZAG_coeff  = _zp10      ; byte, used in _decodeNextMCU
rDMCU          = _zp11      ; byte, used in _decodeNextMCU
sDMCU          = _zp12      ; byte, used in _decodeNextMCU
iDMCU          = _zp13      ; byte, used in _decodeNextMCU
inputIdx       = _zp11      ; byte, used in idctRows and idctCols
; dw             = _zp9       ; byte, used in imul (IDCT)
; neg            = _zp10      ; byte, used in imul (IDCT)

NO_FF_CHECK = $60
FF_CHECK_ON = $EA

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
@still_neg:
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
done:
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
; x = ptr2/+1, sDMCU = A
; Returns in AX or updates ptr2/+1 according to parameter
.macro HUFFEXTEND UPDATEPTR2
.scope
        cmp     #16
        bcs     retNormal

        tax
        lda     _extendTests_h,x

        cmp     ptr2+1
        bcc     retNormal
        bne     retExtend

        lda     ptr2
        cmp     _extendTests_l,x
        bcs     retNormalX
        sec
retExtend:
        lda     _extendOffsets_l,x    ; Carry set here
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
        lda     ptr2
retNormalX:
        ldx     ptr2+1
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

_getBitsNoFF:
        ldx     #NO_FF_CHECK
        jmp     getBits
_getBitsFF:
        ldx     #FF_CHECK_ON
getBits:
        stx     ffcheck
getBitsDirect:
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
        rts

large_n:
        lda     n_min_eight,x ; How much more than 8?
        sta     last_few+1

        lda     #0            ; Init result
        sta     bbHigh

        ldx     #8            ; Get the first eight
:       INLINE_GETBIT_COUNT_Y 1
        rol     a
        dex
        bne     :-

last_few:
        ldx     #$FF          ; Get the last (n-8)
:       INLINE_GETBIT_COUNT_Y 1
        rol     a
        rol     bbHigh
        dex
        bne     :-
        sty     _gBitsLeft

        ldx     bbHigh
        rts

skipBitsDirect:
        ldy     _gBitsLeft
        tax
        cpx     #9
        bcs     skip_large_n
skip_small_n:
:       INLINE_GETBIT_COUNT_Y
        dex
        bne     :-
        sty     _gBitsLeft
        ; X is zero
        rts

skip_large_n:
        lda     n_min_eight,x ; How much more than 8?
        sta     skip_last_few+1

        ldx     #8            ; Get the first eight
:       INLINE_GETBIT_COUNT_Y
        dex
        bne     :-

skip_last_few:
        ldx     #$FF          ; Get the last (n-8)
:       INLINE_GETBIT_COUNT_Y
        dex
        bne     :-
        sty     _gBitsLeft
        rts


AXBCK: .res 2
inc_cache_high1:
        inc     _cur_cache_ptr+1
        ldy     _cur_cache_ptr+1
        cpy     #(>CACHE_END)-1
        bne     :+
start_floppy_motor_a:
        sta     motor_on         ; Start drive motor in advance (patched if on floppy)
:       cpy     #>CACHE_END
        bne     ffcheck
        sta     AXBCK
        stx     AXBCK+1          ; Backup X for caller
        jsr     _fillInBuf
        ldx     AXBCK+1
        lda     AXBCK
        jmp     ffcheck

inc_cache_high2:
        inc     _cur_cache_ptr+1
        ldy     _cur_cache_ptr+1
        cpy     #(>CACHE_END)-1
        bne     :+
start_floppy_motor_b:
        sta     motor_on         ; Start drive motor in advance
:       cpy     #>CACHE_END
        bne     getOctet_done
        sta     AXBCK
        stx     AXBCK+1          ; Backup Y for caller
        jsr     _fillInBuf
        ldx     AXBCK+1
        lda     AXBCK
        ldy     #$00
        jmp     getOctet_done

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

ffcheck:rts                   ; Should we check for $FF? patched.
        cmp     #$FF          ; Is result FF?
        beq     :+
        rts
:       sta     tmp1          ; Remember result

        ; Yes. Read again.
        ldy     #$00
        lda     (_cur_cache_ptr),y

        cmp     #$00          ; is it 0 ?
        beq     out

        ; No, stuff back char
        lda     _cur_cache_ptr
        beq     dec_cache_high
        dec     _cur_cache_ptr

stuff_back:
        ldy     #$00
        lda     #$FF
        sta     (_cur_cache_ptr),y
        lda     tmp1
        rts
out:                          ; We read 0 so increment pointer
        inc     _cur_cache_ptr
        beq     inc_cache_high2
        ; Return result
getOctet_done:
        lda     tmp1
        rts

; Low byte in Y, high in X, high loaded last !
.macro imul TABL, TABM, TABH, REVERSESIGN
.scope
        clc

        ; Check if positive
        stx     neg
        bpl     :+
        tya
        ; val = -val;
        eor     #$FF
        adc     #1

        tay                   ; Set our pos low byte
        txa                   ; Get high byte
        eor    #$FF
        adc    #0
        tax
:
        ; We now have high byte in X, low in Y
        ; dw = mul362_l[l] | mul362_m[l] <<8 | mul362_h[l] <<16;
        ; lda    TABL,y
        ; sta    dw
        ; lda    _mul362_m,y - shortcut right below
        ; sta    tmp1
        ; lda    _mul362_h,y
        ; sta    tmp2

        ; dw += (mul362_l[h]) << 8;
        lda    TABM,y    ; tmp1
        adc    TABL,x
        sta    tmp1

        ; dw += (mul362_m[h]) << 16;
        .ifnblank TABH
        lda    TABH,y
        .else
        lda    #$00
        .endif
        adc    TABM,x
        tax             ; remember for return or sign reversal

        ; Was val negative?
neg = *+1
        lda    #$FF
        .ifblank REVERSESIGN
        bpl    done
        .else
        bmi    done
        .endif
        ; dw ^= 0xffffff, dw++
        lda    #$FF
        eor    TABL,y
        adc    #1
        lda    #$FF
        eor    tmp1
        adc    #0

        sta    tmp1
        txa
        eor    #$FF
        adc    #0
        tax
done:
        lda    tmp1
.endscope
.endmacro

; uint16 __fastcall__ imul_b1_b3(int16 w)
.macro IMUL_B1_B3
        imul    _mul362_l, _mul362_m, _mul362_h
.endmacro

; uint16 __fastcall__ imul_b4(int16 w)
.macro IMUL_B4
        imul    _mul473_l, _mul473_m, _mul473_h, 1
.endmacro

; uint16 __fastcall__ imul_b5(int16 w)
.macro IMUL_B5
        imul    _mul196_l, _mul196_m, , 1
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
        sty     _zp13
        ; x *= gWinogradQuant[i];
        lda     _gWinogradQuant,y
        ldx     #$00
        jsr     push0ax
        ldy     _zp13
        lda     TABL,y
        ldx     TABH,y
        jsr     tosmul0ax

        ; r = (int16)((x + 4) >> (3));
        ldy     #4
        jsr     inceaxy
        jsr     asreax3

        ldy     _zp13
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

        ; x32 = imul_b1_b3(x13) - x13;
        ldy    rx13l
        ldx    rx13h
        IMUL_B1_B3
        sec
        sbc    rx13l
        sta    rx32l
        txa
        sbc    rx13h
        sta    rx32h

        ; res1 = imul_b5(x5);
        ldy    rx5l
        ldx    rx5h
        IMUL_B5
        sta    rres1l
        stx    rres1h

        ; res2 = imul_b4(x5) + x5;
        ldy    rx5l
        ldx    rx5h
        IMUL_B4
        clc                     ; +x5
        adc    rx5l
        sta    rres2l
        txa
        adc    rx5h
        sta    rres2h

        ; res3 = imul_b1_b3(x5) + res2;
        ldy    rx5l
        ldx    rx5h
        IMUL_B1_B3
        clc
rres2l = *+1
        adc    #$FF
        tay
        txa
rres2h = *+1
        adc    #$FF
        tax

        ; *(rowSrc_1) = res3 + x30 - x32;
        tya
        clc
        adc    rx30l
        sta    res3x30l
        tay
        txa
        adc    rx30h
        sta    res3x30h
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

        ; *(rowSrc_2) = res3x30l + res1 - x13;

        clc
res3x30l = *+1
        lda    #$FF
        adc    rres1l
        tay
res3x30h = *+1
        lda    #$FF
        adc    rres1h
        tax

        tya
        sec
        sbc    rx13l

        ldy    inputIdx
        sta    _gCoeffBuf+4,y
        txa
        sbc    rx13h
        sta    _gCoeffBuf+5,y

        ; *(rowSrc_3) = x30 + x32 + res2;
        clc
        lda    rx30l
        adc    rx32l
        sta    tmp1
        lda    rx30h
        adc    rx32h
        tax
        lda    tmp1
        clc
        adc    rres2l
        sta    _gCoeffBuf+6,y
        txa
        adc    rres2h
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
        ldx     _gCoeffBuf,y

        SHIFT_XA_7RIGHT_AND_CLAMP

        sta     val0
        sta     val1
        ; Val2 is A in cont_idct_cols
        sta     val3
        jmp     cont_idct_cols

full_idct_cols:

cx30l = ptr2
cx30h = ptr2+1
cx5l  = ptr3
cx5h  = ptr3+1
cx12l = ptr4
cx12h = ptr4+1

        sty     inputIdx          ; Backup before b5/b2/b4/b1_b3 mults

        ; cx30
        lda     _gCoeffBuf,y
        sta     cx30l
        lda     _gCoeffBuf+1,y
        sta     cx30h

        ; cx5
        lda     _gCoeffBuf+16,y
        sta     cx5l
        lda     _gCoeffBuf+17,y
        sta     cx5h

        ; cx12
        lda     _gCoeffBuf+32,y
        sta     cx12l             ; used 4 times
        lda     _gCoeffBuf+33,y
        sta     cx12h

        ;res1 = imul_b5(x5)
        ldy     cx5l
        ldx     cx5h
        IMUL_B5
        sta     cres1l
        stx     cres1h

        ;res2 = imul_b4(x5) + x5;
        ldy     cx5l
        ldx     cx5h
        IMUL_B4

        clc
        adc     cx5l
        sta     cres2l
        txa
        adc     cx5h
        sta     cres2h

        ;res3 = imul_b1_b3(x5) + res2;
        ldy     cx5l
        ldx     cx5h
        IMUL_B1_B3
        clc
cres2l = *+1
        adc     #$FF
        sta     cres3l
        txa
cres2h = *+1
        adc     #$FF
        sta     cres3h

        ; val0 = ((x30 + x12 + x5) >> PJPG_DCT_SCALE_BITS) +128;
        clc
        ldx     #0
        lda     cx30l
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

        ; cx32 = imul_b1_b3(cx12) - cx12;
        ldy     cx12l
        ldx     cx12h
        IMUL_B1_B3
        sec
        sbc     cx12l
        sta     cx32l
        tay
        txa
        sbc     cx12h
        sta     cx32h
        tax

        ; val3 = ((x32 + x30 + res2) >> PJPG_DCT_SCALE_BITS) +128;
        clc
        tya
        ldy     #0
        adc     cx30l
        bcc     :+
        iny
        clc
:       adc     cres2l
        sta     tmp1

        txa
        adc     cx30h
        cpy     #1
        adc     cres2h
        ldy     tmp1

        SHIFT_YA_7RIGHT_AND_CLAMP
        sta     val3

        ; val1 = ((cx30 + cres3 - cx32) >> PJPG_DCT_SCALE_BITS) +128;
        clc
        lda     cx30l
cres3l = *+1
        adc     #$FF
        sta     cx30lcres3l
        tay

        lda     cx30h
cres3h = *+1
        adc     #$FF
        sta     cx30hcres3h
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

        ;cx30 + cres3 + cres1
        clc
cres1l = *+1
        lda     #$FF
cx30lcres3l = *+1
        adc     #$FF
        tay
cres1h = *+1
        lda     #$FF
cx30hcres3h = *+1
        adc     #$FF
        tax

        ; -x12
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
        bne     :+
        inc     _output0+1
        inc     _output1+1
        inc     _output2+1
        inc     _output3+1
:       sty     _outputIdx

        ldy     inputIdx

        cpy     #(3*2)
        beq     idctColDone
        iny
        iny
        ; inc     outputIdx - already inc'd while loaded
        jmp     nextCol

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
        beq     :+
        jsr     getBitsDirect ; r = getBitsFF(numExtraBits);
        .byte   $A0           ; ldy IMM, eats tax, eq jmp doExtend
:       tax                   ; otherwise set r to uint16 0 (A is 0)

        sta     ptr2         ; dc = huffExtend(r, s);
        stx     ptr2+1
        lda     sDMCU
        HUFFEXTEND            ; DC in AX now

        ; dc = dc + gLastDC[componentID=0];
        ; gLastDC[componentID=0] = dc;
        clc
        adc     _gLastDC_l
        sta     _gLastDC_l
        sta     ptr2          ; Store dc to ptr2 as it's where mult expects it

        txa
        adc     _gLastDC_h
        sta     _gLastDC_h
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
        tay                     ; to Y in case !s

        txa                     ; restore value, numExtraBits = s & 0xF
        and     #$0F

        bne     getData         ; if (s)
        lda     rDMCU
        cmp     #15
        beq     checkZAGLoop
        jmp     ZAG_finished

getData:
        sta     dataS+1
        jsr     getBitsDirect   ; extraBits = getBitsFF(numExtraBits);
        stx     ptr2+1          ; Store for huffExtend

        ldy     cur_ZAG_coeff   ; We only do a part of the matrix
        ldx     _ZAG_Coeff,y
        bmi     end_of_coeff_calc
        stx     ZC              ; Remember Zag coeff
        sta     ptr2            ; Finish storing for huffExtend

        ;ac = huffExtend(sDMCU)
dataS:
        lda     #$FF
        HUFFEXTEND 1

        ;gCoeffBuf[cur_ZAG_coeff] = ac * pQ[cur_ZAG_coeff];
load_pq1l:
        lda     $FFFF,y         ; Y still cur_ZAG_coeff
coeff_calc:
        jmp     mult_coeff_16
coeff_calc_done:

ZC = *+1
        ldy     #$FF            ; ZAG_Coeff not cur_ZAG_coeff
        sta     _gCoeffBuf,y
        txa
        sta     _gCoeffBuf+1,y

        ldy     cur_ZAG_coeff

end_of_coeff_calc:
        iny                     ; cur_ZAG_coeff
checkZAGLoop:
        cpy     #64             ; end_ZAG_coeff
        bne     doZAGLoop

ZAG_finished:
        jsr     _idctRows
        jsr     _idctCols

        inc     mcuBlock
        ldx     mcuBlock
        cpx     #2
        bcs     nextUselessBlock
        jmp     nextMcuBlock

nextUselessBlock:
        ; Skip the other blocks, do the minimal work
        stx     mcuBlock

uselessDecDC:
        jmp     $FFFF
skipDC:
        and     #$0F
        beq     :+
        jsr     skipBitsDirect

:       lda     #1
i64loop:
        ;for (iDMCU = 1; iDMCU != 64; iDMCU++) {
        sta     iDMCU
uselessDecAC:
        jmp     $FFFF   ; Patched with huffDecode2/3
skipAC:
        beq     ZAG2_Done
        and     #$0F
        sta     tmp1
        jsr     skipBitsDirect
        ldx     tmp1
        lda     right_shift_4,x

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
