
        .importzp   tmp1, tmp2, tmp3, tmp4, ptr1, ptr2, ptr3, _prev_ram_irq_vector, _prev_rom_irq_vector, c_sp, ptr4
        .importzp   _zp2, _zp4, _zp6, _zp8, _zp7, _zp9, _zp10, _zp11, _zp12, _zp13
        .import     popptr1
        .import     _extendTests_l, _extendTests_h, _extendOffsets_l, _extendOffsets_h
        .import     _fillInBuf, _cache
        .import     _mul669_l, _mul669_m, _mul669_h
        .import     _mul362_l, _mul362_m, _mul362_h
        .import     _mul277_l, _mul277_m, _mul277_h
        .import     _mul196_l, _mul196_m, _mul196_h
        .import     _gCoeffBuf, _gRestartInterval, _gRestartsLeft
        .import     _gMaxBlocksPerMCU, _processRestart, _gCompACTab, _gCompQuant
        .import     _gQuant0_l, _gQuant1_l, _gQuant0_h, _gQuant1_h
        .import     _gCompDCTab, _gMCUOrg, _gLastDC_l, _gLastDC_h, _gCoeffBuf, _ZAG_Coeff
        .import     _gHuffTab0, _gHuffVal0, _gHuffTab1, _gHuffVal1, _gHuffTab2, _gHuffVal2, _gHuffTab3, _gHuffVal3
        .import     _gMCUBufG
        .import     _gNumMCUSRemainingX, _gNumMCUSRemainingY
        .import     _gWinogradQuant
        .import     floppy_motor_on

        .import     asreax3, inceaxy, tosmul0ax, push0ax
        .import     shraxy, decsp4, popax, addysp, mult16x16x16_direct
        .export     _huffExtend, _getBitsNoFF, _getBitsFF
        .export     _imul_b1_b3, _imul_b2, _imul_b4, _imul_b5
        .export     _idctRows, _idctCols, _decodeNextMCU, _transformBlock
        .export     _pjpeg_decode_mcu
        .export     _copy_decoded_to
        .export     _createWinogradQuant0, _createWinogradQuant1
        .export     _initFloppyStarter

; ZP vars. Mind that qt-conv uses some too
_gBitBuf       = _zp2       ; word, used everywhere
code           = _zp4       ; word, used in huffDecode

; zp6-7 USED in qt-conv so only use it temporarily
mcuBlock       = _zp7       ; byte, used in _decodeNextMCU

_gBitsLeft     = _zp8       ; byte, used everywhere
cur_pQ         = _zp9       ; byte, used in _decodeNextMCU
cur_ZAG_coeff  = _zp10      ; byte, used in _decodeNextMCU
rDMCU          = _zp11      ; byte, used in _decodeNextMCU
sDMCU          = _zp12      ; byte, used in _decodeNextMCU
iDMCU          = _zp13      ; byte, used in _decodeNextMCU

dw             = _zp9       ; byte, used in imul (IDCT)
neg            = _zp10      ; byte, used in imul (IDCT)

NO_FF_CHECK = $60
FF_CHECK_ON = $EA

CACHE_END = _cache + CACHE_SIZE + 4
.assert <CACHE_END = 0, error

.struct hufftable_t
   mMinCode_l .res 16
   mMaxCode_l .res 16
   mMaxCode_h .res 16
   mValPtr    .res 16
   mGetMore   .res 16
.endstruct

_cur_cache_ptr = _prev_ram_irq_vector

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
; Returns with A = 0 and carry set if bit 1
.macro INLINE_GETBIT
.scope
        dec     _gBitsLeft
        bpl     haveBit

        jsr     getOctet
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
retNormalX:
        ldx     extendX+1
        rts

:       tax
        ldy     _extendTests_h,x

        cpy     extendX+1
        bcc     retNormal
        bne     retExtend

        lda     extendX
        cmp     _extendTests_l,x
        bcs     retNormalX
        sec
retExtend:
        lda     _extendOffsets_l,x    ; Carry set here
        adc     extendX
        tay
        lda     _extendOffsets_h,x
        adc     extendX+1
        tax
        tya
        rts

; #define getBitsNoFF(n) getBits(n, 0)
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

.macro GET_BITS_SET_FF_ON
        ldx     #FF_CHECK_ON
        stx     ffcheck
.endmacro

; ===== Rare case when we need more than 8 bits,
; ===== Moved out of the main codepath
need_more_than_eight:
        ldx     n_min_eight,y
        stx     n

        ; gBitBuf <<= gBitsLeft;
        ldy     _gBitsLeft
        beq     no_lshift

        cpy     #8
        bcc     :+
        sta     _gBitBuf+1
        jmp     no_lshift

        ; lda     _gBitBuf - A already contains _gBitBuf
:       asl     a
        rol     _gBitBuf+1
        dey
        bne     :-
        ;sta     _gBitBuf - will be overwritten
no_lshift:

        ; gBitBuf |= getOctet(ff);
        jsr     getOctet
        sta     _gBitBuf

        ; gBitBuf <<= (8 - gBitsLeft);
        ldx     _gBitsLeft
        ldy     eight_min_n,x
        beq     n_lt8         ; Back to main codepath
        cpy     #8
        bne     :+
        ; lda     _gBitBuf  - already contains gBitBuf
        sta     _gBitBuf+1
        ; lda     #$00      - no need to store, getOctet'd later
        ; sta     _gBitBuf
        jmp     n_lt8         ; Back to main codepath

:       ; lda     _gBitBuf  - already contains gBitBuf
        asl     a
        rol     _gBitBuf+1
        dey
        bne     :-
        sta     _gBitBuf
        lda     _gBitBuf+1  ; ret = (ret & 0xFF00) | (gBitBuf >> 8);
        jmp     n_lt8
; =====================================

_getBitsNoFF:
        ldx     #NO_FF_CHECK
        jmp     getBits
_getBitsFF:
        ldx     #FF_CHECK_ON
getBits:
        stx     ffcheck
getBitsDirect:
        sta     n
        tay
        lda     sixteen_min_n,y
        sta     final_shift

        lda     _gBitBuf+1
        sta     ret+1
        lda     _gBitBuf      ; Will be stored later

        cpy     #9
        bcs     need_more_than_eight

n_lt8:
        sta     ret

        ; if (gBitsLeft < n) {
        ldy     _gBitsLeft
        beq     no_lshift3
        cpy     n
        bcs     enoughBits

        ; gBitBuf <<= gBitsLeft;
        lda     _gBitBuf
        cpy     #8
        bne     :+
        sta     _gBitBuf+1
        jmp     no_lshift3
:       asl     a
        rol     _gBitBuf+1
        dey
        bne     :-
        ;sta     _gBitBuf - will get overwritten by getOctet
no_lshift3:

        ; gBitBuf |= getOctet(ff);
        jsr     getOctet
        sta     _gBitBuf

        ; tmp = n - gBitsLeft;
        lda     n
        sec
        sbc     _gBitsLeft
        tay
        beq     no_lshift4

        ; gBitBuf <<= tmp;
        cmp     #8
        bne     :+
        lda     _gBitBuf
        sta     _gBitBuf+1
        jmp     no_lshift4

:       tax                   ; Keep Y = tmp
        lda     _gBitBuf
:       asl     a
        rol     _gBitBuf+1
        dex
        bne     :-
        sta     _gBitBuf

no_lshift4:
        ; gBitsLeft = 8 - tmp;
        lda     eight_min_n,y
        sta     _gBitsLeft
        jmp     left_shifts_done

enoughBits:
        ; gBitsLeft = gBitsLeft - n;
        tya                   ; - contains gBitsLeft
        sbc     n
        sta     _gBitsLeft

        lda     _gBitBuf
        ldy     n
        cpy     #8
        bne     :+
        sta     _gBitBuf+1
        jmp     left_shifts_done

        ; gBitBuf <<= n;
:       asl     a
        rol     _gBitBuf+1
        dey
        bne     :-
        sta     _gBitBuf

left_shifts_done:
        ; return ret >> final_shift
        ldy     final_shift
        beq     load_res
        cpy     #8
        bcs     :++

        lda     ret             ; << less than 8, long way
:       lsr     ret+1
        ror     a
        dey
        bne     :-
        ldx     ret+1
        rts

:       lda     ret+1           ; << 8 or more, fast way
        ldx     n_min_eight,y
        beq     no_final_rshift
:       lsr     a
        dex
        bne     :-
        ; Now X is 0
        ; ldx   #0
no_final_rshift:
        rts
load_res:
        lda     ret
        ldx     ret+1
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
        stx     AXBCK+1          ; Backup Y for caller
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

.macro imul TABL, TABM, TABH
.scope
        tay                   ; val low byte in Y

        stx     neg
        cpx     #$80
        bcc     :+

        ; val = -val;
        ; clc                 ; carry is set
        eor     #$FF
        ; adc     #1
        adc    #0

        tay
        txa
        eor    #$FF
        adc    #0
        tax

:       stx    tmp4
        ; dw = mul362_l[l] | mul362_m[l] <<8 | mul362_h[l] <<16;
        ; lda    TABL,y
        ; sta    dw
        ; lda    _mul362_m,y - shortcut right below
        ; sta    tmp1
        ; lda    _mul362_h,y
        ; sta    tmp2

        ; dw += (mul362_l[h]) << 8;
        ; clc            - carry is clear
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
        tax             ; remember for return or sign reversal

        ; Was val negative?
        lda    neg
        bmi    :+
        lda    tmp1
        rts

:       ; dw ^= 0xffffffff, dw++
        ; clc             - carry is clear
        lda    #$FF
        eor    TABL,y
        adc    #1
        lda    #$FF
        eor    tmp1
        adc    #0
        tay
        txa
        eor    #$FF
        adc    #0
        tax
        tya
        rts
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

; uint8 __fastcall__ huffDecode(const uint8* pHuffVal)
.macro huffDecode TABLE, VAL
.scope
        INLINE_GETBIT
        sta     code+1  ; A = 0 here
        rol             ; Now A = 0 or 1 depending on bit
        sta     code

        ldx     #16
        stx     tmp1
        ldx     #$00
nextLoop:
        lda TABLE+hufftable_t::mGetMore,x
        bne increment

        lda TABLE+hufftable_t::mMaxCode_h,x
        cmp code+1              ; curMaxCode < code ? hibyte
        bcc increment
        bne loopDone

checkLow:
        lda TABLE+hufftable_t::mMaxCode_l,x
        cmp code                ; ; curMaxCode < code ? lobyte
        bcs loopDone
increment:
        INLINE_GETBIT
        rol     code
        rol     code+1
        inx
        dec     tmp1
        bne     nextLoop

        lda     #$00          ; if i == 16 return 0
        tax
        rts

loopDone:
        lda     TABLE+hufftable_t::mValPtr,x
        adc     code
        sec
        sbc     TABLE+hufftable_t::mMinCode_l,x
        tax                     ; Get index

        lda     VAL,x
        rts
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
        sta    tmp1
        txa
        sbc    _gCoeffBuf+13,y

        ; x32 = imul_b1_b3(x12) - x13;
        tax
        lda     tmp1
        sty     tmp3
        jsr    _imul_b1_b3
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
        tay
        lda    x5+1
        adc    x7+1
        sta    x17+1
        tax

        ;*(rowSrc) = x30 + x13 + x17;
        clc
        tya
        adc    x13
        tay
        txa
        adc    x13+1
        tax
        tya
        clc
        adc    x30
        ldy    tmp3
        sta    _gCoeffBuf,y
        txa
        adc    x30+1
        sta    _gCoeffBuf+1,y

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
        tay
        lda    x5+1
        sbc    x7+1
        tax

        ; res3 = imul_b1_b3(x15) - res2;
        tya
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
        ldy     tmp3          ; and restore
        sta    _gCoeffBuf+4,y
        txa
        sbc    x32+1
        sta    _gCoeffBuf+5,y

        ; *(rowSrc_4) = x30 + res3 + x24 - x13;
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
        ldy    tmp3
        sta    _gCoeffBuf+8,y
        txa
        sbc    x13+1
        sta    _gCoeffBuf+9,y

        ; *(rowSrc_6) = x31 + x32 - res2;
        lda    x31
        clc
        adc    x32
        sta    tmp1
        lda    x31+1
        adc    x32+1
        tax
        lda    tmp1
        sec
        sbc    res2
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

        eor     #$80
        bmi     :+
        inx
:       cpx     #$00
        beq     clampDone1
        txa                     ; Clamp:
        asl                     ; get sign to carry
        lda     #$FF            ; $FF+C = 0 if neg, $FF+c = $FF if pos
        adc     #0
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

        sty     tmp3          ; Backup before b5/b2/b4/b1_b3 mults

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
        tay
        lda     x5+1
        sbc     x7+1
        tax
        tya
        jsr     _imul_b1_b3
        sec
        sbc     res2
        sta     res3
        txa
        sbc     res2+1
        sta     res3+1

        ldy     tmp3          ; And restore
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

        ;x32 = imul_b1_b3(x12) - x13;
x12l:
        lda     #$FF
x12h:
        ldx     #$FF
        ; sty     tmp3          ; Backup before mult
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
        tay
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
        tya
        clc
        adc     x17
        tay
        txa
        adc     x17+1

        tax
        tya
        INLINE_ASRAX7

        eor     #$80
        bmi     :+
        inx

:       cpx     #$00
        beq     clampDone2
        txa                     ; Clamp:
        asl                     ; get sign to carry
        lda     #$FF            ; $FF+C = 0 if neg, $FF+c = $FF if pos
        adc     #0
clampDone2:
        ldy     tmp3            ; And restore
        sta     _gCoeffBuf,y

        ;x42 = x31 - x32;
        sec
        lda     x31
        sbc     x32
        tay
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
        tya
        adc     res3
        tay
        txa
        adc     res3+1

        tax
        tya
        INLINE_ASRAX7

        eor     #$80
        bmi     :+
        inx
:       cpx     #$00
        beq     clampDone3
        txa                     ; Clamp:
        asl                     ; get sign to carry
        lda     #$FF            ; $FF+C = 0 if neg, $FF+c = $FF if pos
        adc     #0
clampDone3:
        ldy     tmp3            ; And restore
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
        tay
        lda     res3+1
x24h:
        adc     #$FF
        tax

        ; t = ((x43 + x44) >> PJPG_DCT_SCALE_BITS) +128;
        tya
        clc
x43l:
        adc     #$FF
        tay
        txa
x43h:
        adc     #$FF

        tax
        tya
        INLINE_ASRAX7

        eor     #$80
        bmi     :+
        inx
:       cpx     #$00
        beq     clampDone4
        txa                     ; Clamp:
        asl                     ; get sign to carry
        lda     #$FF            ; $FF+C = 0 if neg, $FF+c = $FF if pos
        adc     #0
clampDone4:
        ldy     tmp3            ; And restore
        sta     _gCoeffBuf+64,y

        ;x41 = x31 + x32;
        lda     x32
        clc
        adc     x31
        tay
        lda     x32+1
        adc     x31+1
        tax

        ; t = ((x41 - res2) >> PJPG_DCT_SCALE_BITS) +128;
        tya
        sec
        sbc     res2
        tay
        txa
        sbc     res2+1

        tax
        tya
        INLINE_ASRAX7

        eor     #$80
        bmi     :+
        inx
:       cpx     #$00
        beq     clampDone5
        txa                     ; Clamp:
        asl                     ; get sign to carry
        lda     #$FF            ; $FF+C = 0 if neg, $FF+c = $FF if pos
        adc     #0
clampDone5:
        ldy     tmp3            ; And restore
        sta     _gCoeffBuf+96,y

cont_idct_cols:
        dec     idctCC
        beq     idctColDone
        iny
        iny
        jmp     nextCol

idctColDone:
        rts

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
        ldy     _gMCUOrg,x
        sty     componentID

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

        and     #$0F
        beq     :+
        jsr     getBitsDirect
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

        ldy     componentID

        ;compACTab = gCompACTab[componentID];
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

        ; dc = dc + gLastDC[componentID];
        ; gLastDC[componentID] = dc;
        clc
        lda     _gLastDC_l,y
        adc     tmp1
        sta     _gLastDC_l,y
        sta     ptr2

        lda     _gLastDC_h,y
        adc     tmp2
        sta     _gLastDC_h,y

        ;gCoeffBuf[0] = dc * pQ[0];
        sta     ptr2+1

load_pq0h:
        ldx     $FFFF
load_pq0l:
        lda     $FFFF
        jsr     mult16x16x16_direct
        sta     _gCoeffBuf
        stx     _gCoeffBuf+1

        lda     #1
        sta     cur_ZAG_coeff
        sta     cur_pQ

checkZAGLoop:
        lda     cur_ZAG_coeff
        cmp     #64             ; end_ZAG_coeff
        beq     ZAG_Done

doZAGLoop:
huffDec:
        jsr     $FFFF           ; Patched with huffDecode2/3
        tay
        lsr     a
        lsr     a
        lsr     a
        lsr     a
        sta     rDMCU
        tya
        and     #$0F
        sta     sDMCU

        beq     :+              ; if numExtraBits (s & 0x0F)
        jsr     getBitsDirect
        .byte   $A0             ; LDY imm opcode, skips tax, = jmp     storeExtraBits
:       tax                     ; extraBits = 0

storeExtraBits:
        sta     extendX         ; Store for later huffExtending
        stx     extendX+1

        lda     sDMCU           ; s = numExtraBits

        beq     sZero

        lda     rDMCU
        beq     zeroZAGDone

        ; while (r) { gCoeffBuf[*cur_ZAG_coeff] = 0; ...}
        ldx     cur_ZAG_coeff
        lda     #0
zeroZAG:
        ldy     _ZAG_Coeff,x
        sta     _gCoeffBuf,y
        sta     _gCoeffBuf+1,y

        inx
        inc     cur_pQ

        dec     rDMCU
        bne     zeroZAG
        stx     cur_ZAG_coeff

zeroZAGDone:
        ;ac = huffExtend(sDMCU)
        ; extendX already set
        lda     sDMCU
        jsr     _huffExtend

        ;gCoeffBuf[*cur_ZAG_coeff] = ac * *cur_pQ;
        sta     ptr2
        stx     ptr2+1

        ldy     cur_pQ
load_pq1l:
        lda     $FFFF,y
load_pq1h:
        ldx     $FFFF,y
        jsr     mult16x16x16_direct

        sta    tmp1

        txa
        ldx     cur_ZAG_coeff
        ldy     _ZAG_Coeff,x
        sta     _gCoeffBuf+1,y
        lda    tmp1
        sta     _gCoeffBuf,y
        jmp     sNotZero

; Inserted here, in an otherwise unreachable place,
; to be more easily reachable from everywhere we need it
ZAG_Done:
        ldx     cur_ZAG_coeff
        lda     #0
finishZAG:
        cpx     #64             ; end_ZAG_coeff
        beq     ZAG_finished

        ldy     _ZAG_Coeff,x
        sta     _gCoeffBuf,y
        sta     _gCoeffBuf+1,y

        inx
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

jmp_nextMcuBlock:
        jmp     nextMcuBlock

ZAG_finished:
        stx     cur_ZAG_coeff ; Store cur_ZAG_Coeff after looping in finishZAG
        lda     mcuBlock
        jsr     _transformBlock

        inc     mcuBlock
        ldx     mcuBlock
        cpx     #2
        bcc     jmp_nextMcuBlock

firstMCUBlocksDone:
        ; Skip the other blocks, do the minimal work
        ldx     mcuBlock
        cpx     _gMaxBlocksPerMCU
        bcs     uselessBlocksDone

nextUselessBlock:
        ldy     _gMCUOrg,x
        sty     componentID

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

        lda     _gCompDCTab,y
        beq     :+

        jsr     _huffDecode1
        jmp     doDecb

:       jsr     _huffDecode0

doDecb:
        and     #$0F
        beq     :+
        jsr     getBitsDirect

:       lda     #1
        sta     iDMCU
i64loop:
        ;for (iDMCU = 1; iDMCU != 64; iDMCU++) {
uselessDec:
        jsr     $FFFF   ; Patched with huffDecode2/3

        sta     sDMCU
        and     #$0F
        beq     ZAG2_Done
        sta    tmp1
        jsr     getBitsDirect
        lda    tmp1

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
        ldx     mcuBlock
        cpx     _gMaxBlocksPerMCU
        bcc     nextUselessBlock

uselessBlocksDone:
        lda     #$00
        tax
        rts

_transformBlock:
        pha

        jsr     _idctRows
        jsr     _idctCols

        ldy     #31

        pla
        beq     :+
        ldy     #(32+31)

:       ldx     #(31*4)
        sec
tbCopy:
        lda     _gCoeffBuf,x
        sta     _gMCUBufG,y

        dey
        txa
        sbc     #4
        tax
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

_copy_decoded_to:
        sta    ptr1     ; pDst1 = pDst_row
        stx    ptr1+1
        stx    ptr2+1
        clc
        adc    #(8>>1)  ; pDst2 = pDst_row + 8>>1
        sta    ptr2
        bcc    :+
        inc    ptr2+1
        clc             ; Make sure to clear carry

:       ldx    #4       ; by = 4
        ldy    #0       ; s = 0

copy_cont:
        lda    _gMCUBufG,y
        sta    (ptr1),y
        lda    _gMCUBufG+32,y
        sta    (ptr2),y
        iny
        lda    _gMCUBufG,y
        sta    (ptr1),y
        lda    _gMCUBufG+32,y
        sta    (ptr2),y
        iny
        lda    _gMCUBufG,y
        sta    (ptr1),y
        lda    _gMCUBufG+32,y
        sta    (ptr2),y
        iny
        lda    _gMCUBufG,y
        sta    (ptr1),y
        lda    _gMCUBufG+32,y
        sta    (ptr2),y

        dex
        beq    copy_out

        lda    ptr1    ; pDst1 += (DECODED_WIDTH);
        adc    #<(320-8) ; Carry sure to be clear
        sta    ptr1
        lda    ptr1+1
        adc    #>(320-8)
        sta    ptr1+1

        lda    ptr2   ; pDst2 += (DECODED_WIDTH);
        adc    #<(320-8)
        sta    ptr2
        lda    ptr2+1
        adc    #>(320-8)
        sta    ptr2+1

        tya           ; s += 4+1
        adc    #5
        tay

        jmp    copy_cont
copy_out:
        rts

        .bss

;getBit/octet
n:      .res 1
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
componentID:.res 1
