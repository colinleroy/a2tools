
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
   mMaxCode_l .res 16
   mMaxCode_h .res 16
   mValPtr    .res 16
   mGetMore   .res 16
.endstruct

_cur_cache_ptr = _prev_ram_irq_vector

; high in X low in A
.macro INLINE_ASRAX7            ; X reg.   A reg.
.scope                          ; HXXXXXXL hAAAAAAl
        asl                     ;          AAAAAAA0, h->C
        txa
        rol                     ;          XXXXXXLh, H->C
        ldx     #$00            ; 00000000 XXXXXXLh
        bcc     @done
        dex                     ; 11111111 XXXXXXLh if C
@done:
.endscope
.endmacro

; high in A low in X
.macro INLINE_ASRXA7            ; A reg    X reg
.scope                          ; HXXXXXXL hAAAAAAl
        cpx     #$80            ; HXXXXXXL hAAAAAAl,  h->C
        rol                     ; XXXXXXLh no_care , H->C
        ldx     #$00            ; XXXXXXLh 00000000
        bcc     @done
                                ; X reg    A reg
        dex                     ; 11111111 XXXXXXLh if C
@done:
.endscope
.endmacro

; high in A low in Y
.macro INLINE_ASRYA7            ; A reg    Y reg
.scope                          ; HXXXXXXL hAAAAAAl
        cpy     #$80            ; HXXXXXXL hAAAAAAl,  h->C
        rol                     ; XXXXXXLh no_care , H->C
        ldx     #$00            ; XXXXXXLh no_care
        bcc     @done
                                ; X reg    A reg
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
; Returns with carry set if bit 1
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

; Low byte in Y, high in X, high loaded last !
.macro imul TABL, TABM, TABH
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

:       ; dw ^= 0xffffff, dw++
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
        lda     #0
        sta     code+1  ; init code+1 for long search
        rol             ; Now A = 0 or 1 depending on bit
        sta     code

        ldx     #$00
nextLoopS:
        ldy     TABLE+hufftable_t::mGetMore,x
        bne     incrementS

        lda     TABLE+hufftable_t::mMaxCode_l,x
        cmp     code
        bcs     loopDone
incrementS:
        INLINE_GETBIT
        rol     code
        inx
        cpx     #7
        bne     nextLoopS
        jmp     decodeLong
loopDone:
        ; carry set, ValPtr values decremented by 1
        ; to spare two cycles
        lda     TABLE+hufftable_t::mValPtr,x
        adc     code
        tax                     ; Get index

        lda     VAL,x
        rts

decodeLong:
nextLoopL:
        lda     TABLE+hufftable_t::mGetMore,x
        bne     incrementL

        lda     TABLE+hufftable_t::mMaxCode_h,x
        cmp     code+1              ; curMaxCode < code ? hibyte
        bcc     incrementL
        bne     loopDone

        lda     TABLE+hufftable_t::mMaxCode_l,x
        cmp     code
        bcs     loopDone
incrementL:
        INLINE_GETBIT
        rol     code
        rol     code+1
        inx
        cpx     #16
        bne     nextLoopL

        lda     #$00          ; if i == 16 return 0
        tax
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
        sty     tmp3

        clc
        lda    _gCoeffBuf+10,y
        adc    _gCoeffBuf+6,y
        sta    x7l
        lda    _gCoeffBuf+11,y
        tax
        adc    _gCoeffBuf+7,y
        sta    x7h

        sec
        lda    _gCoeffBuf+10,y
        sbc    _gCoeffBuf+6,y
        sta    x4l
        txa
        sbc    _gCoeffBuf+7,y
        sta    x4h

        clc
        lda    _gCoeffBuf+2,y
        adc    _gCoeffBuf+14,y
        sta    x5l
        lda    _gCoeffBuf+3,y
        tax
        adc    _gCoeffBuf+15,y
        sta    x5h

        sec
        lda    _gCoeffBuf+2,y
        sbc    _gCoeffBuf+14,y
        sta    x6l
        txa
        sbc    _gCoeffBuf+15,y
        sta    x6h

        clc
        lda    _gCoeffBuf,y
        adc    _gCoeffBuf+8,y
        sta    x30l
        lda    _gCoeffBuf+1,y
        tax
        adc    _gCoeffBuf+9,y
        sta    x30h

        sec
        lda    _gCoeffBuf,y
        sbc    _gCoeffBuf+8,y
        sta    x31l
        txa
        sbc    _gCoeffBuf+9,y
        sta    x31h

        clc
        lda    _gCoeffBuf+4,y
        adc    _gCoeffBuf+12,y
        sta    x13l
        lda    _gCoeffBuf+5,y
        tax
        adc    _gCoeffBuf+13,y
        sta    x13h

        ; x32 = imul_b1_b3(*(rowSrc_2) - *(rowSrc_6)) - x13;
        sec
        lda    _gCoeffBuf+4,y
        sbc    _gCoeffBuf+12,y
        tay
        txa
        ldx    tmp3           ; loading index in X to preserve Y for mult
        sbc    _gCoeffBuf+13,x
        tax
        jsr    _imul_b1_b3

        sec
x13l = *+1
        sbc    #$FF
        sta    x32l
        txa
x13h = *+1
        sbc    #$FF
        sta    x32h

        ; x17 = x5+x7
        clc
x5l = *+1
        lda    #$FF
x7l = *+1
        adc    #$FF
        sta    x17l
        tay
x5h = *+1
        lda    #$FF
x7h = *+1
        adc    #$FF
        sta    x17h
        tax

        ;*(rowSrc) = x30 + x13 + x17;
        clc
        tya
        adc    x13l
        tay
        txa
        adc    x13h
        tax
        tya

        clc
x30l = *+1
        adc    #$FF
        ldy    tmp3
        sta    _gCoeffBuf,y
        txa
x30h = *+1
        adc    #$FF
        sta    _gCoeffBuf+1,y

        ; res1 = imul_b5(x4 - x6);
x4l = *+1
        lda    #$FF
        sec
x6l = *+1
        sbc    #$FF
        tay
x4h = *+1
        lda    #$FF
x6h = *+1
        sbc    #$FF
        tax
        jsr    _imul_b5
        sta    res1l
        stx    res1h

        ldy    x6l
        ldx    x6h
        jsr    _imul_b4

        sec                     ; -res1
        sbc    res1l
        tay
        txa
        sbc    res1h
        tax

        tya
        sec                     ; -x17
x17l = *+1
        sbc    #$FF
        sta    rres2l
        txa
x17h = *+1
        sbc    #$FF
        sta    rres2h

        ; x15 = x5 - x7;
        sec
        lda    x5l
        sbc    x7l
        tay
        lda    x5h
        sbc    x7h
        ; res3 = imul_b1_b3(x15) - res2;
        tax
        jsr    _imul_b1_b3
        sec
rres2l = *+1
        sbc    #$FF
        sta    rres3l
        tay
        txa
rres2h = *+1
        sbc    #$FF
        sta    rres3h
        tax

        ; *(rowSrc_2) = res3 + x31 - x32;
        tya
        clc
x31l = *+1
        adc    #$FF
        tay
        txa
x31h = *+1
        adc    #$FF
        tax
        tya

        sec
x32l = *+1
        sbc    #$FF
        ldy     tmp3          ; and restore
        sta    _gCoeffBuf+4,y
        txa
x32h = *+1
        sbc    #$FF
        sta    _gCoeffBuf+5,y

        ; x24 = res1 - imul_b2(x4);
        ldy    x4l
        ldx    x4h
        jsr    _imul_b2
        sta    tmp1
        stx    tmp2
res1l = *+1
        lda    #$FF

        sec
        sbc    tmp1
        tay
res1h = *+1
        lda    #$FF
        sbc    tmp2
        tax

        ; *(rowSrc_4) = x24 + x30 + res3 - x13;
        clc
        tya
        adc    x30l
        tay
        txa
        adc    x30h
        tax
        tya
        
        clc
rres3l = *+1
        adc    #$FF
        tay
        txa
rres3h = *+1
        adc    #$FF
        tax
        tya

        sec
        sbc    x13l
        ldy    tmp3
        sta    _gCoeffBuf+8,y
        txa
        sbc    x13h
        sta    _gCoeffBuf+9,y

        ; *(rowSrc_6) = x31 + x32 - res2;
        clc
        lda    x31l
        adc    x32l
        sta    tmp1
        lda    x31h
        adc    x32h
        tax
        lda    tmp1
        sec
        sbc    rres2l
        sta    _gCoeffBuf+12,y
        txa
        sbc    rres2h
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
        lda     _gCoeffBuf+1,y
        ldx     _gCoeffBuf,y

        INLINE_ASRXA7

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
        sty     tmp3          ; Backup before b5/b2/b4/b1_b3 mults

        sec
        lda     _gCoeffBuf+80,y
        sbc     _gCoeffBuf+48,y
        sta     cx4l
        lda     _gCoeffBuf+81,y
        tax
        sbc     _gCoeffBuf+49,y
        sta     cx4h

        clc
        lda     _gCoeffBuf+80,y
        adc     _gCoeffBuf+48,y
        sta     cx7l
        txa
        adc     _gCoeffBuf+49,y
        sta     cx7h

        sec
        lda     _gCoeffBuf+16,y
        sbc     _gCoeffBuf+112,y
        sta     cx6l
        lda     _gCoeffBuf+17,y
        tax
        sbc     _gCoeffBuf+113,y
        sta     cx6h

        clc
        lda     _gCoeffBuf+16,y
        adc     _gCoeffBuf+112,y
        sta     cx5l
        txa
        adc     _gCoeffBuf+113,y
        sta     cx5h

        ;x17 = x5 + x7;
        clc
cx5l = *+1
        lda     #$FF
cx7l = *+1
        adc     #$FF
        sta     cx17l
cx5h = *+1
        lda     #$FF
cx7h = *+1
        adc     #$FF
        sta     cx17h

        ;res1 = imul_b5(x4 - x6)
        sec
cx4l = *+1
        lda     #$FF
cx6l = *+1
        sbc     #$FF
        tay
cx4h = *+1
        lda     #$FF
cx6h = *+1
        sbc     #$FF
        tax
        jsr     _imul_b5
        sta     cres1l
        stx     cres1h

        ;stg26 = imul_b4(x6) - res1;
        ;res2 = stg26 - x17;
        ldy     cx6l
        ldx     cx6h
        jsr     _imul_b4

        sec
        sbc     cres1l
        tay
        txa
        sbc     cres1h
        tax
        tya

        sec
cx17l = *+1
        sbc     #$FF
        sta     cres2l
        txa
cx17h = *+1
        sbc     #$FF
        sta     cres2h

        ;x15 = x5 - x7;
        ;res3 = imul_b1_b3(x15) - res2;
        sec
        lda     cx5l
        sbc     cx7l
        tay
        lda     cx5h
        sbc     cx7h
        tax
        jsr     _imul_b1_b3
        sec
cres2l = *+1
        sbc     #$FF
        sta     cres3l
        txa
cres2h = *+1
        sbc     #$FF
        sta     cres3h

        ldy     tmp3          ; And restore
        sec
        lda     _gCoeffBuf,y
        sbc     _gCoeffBuf+64,y
        sta     cx31l
        lda     _gCoeffBuf+1,y
        tax
        sbc     _gCoeffBuf+65,y
        sta     cx31h

        clc
        lda     _gCoeffBuf,y
        adc     _gCoeffBuf+64,y
        sta     cx30l
        txa
        adc     _gCoeffBuf+65,y
        sta     cx30h

        clc
        lda     _gCoeffBuf+32,y
        adc     _gCoeffBuf+96,y
        sta     cx13l
        lda     _gCoeffBuf+33,y
        tax
        adc     _gCoeffBuf+97,y
        sta     cx13h

        sec
        lda     _gCoeffBuf+32,y
        sbc     _gCoeffBuf+96,y
        tay
        txa
        ldx     tmp3              ; preserve Y for mult
        sbc     _gCoeffBuf+97,x
        tax
        jsr     _imul_b1_b3
        sec
cx13l = *+1
        sbc     #$FF
        sta     cx32l
        txa
cx13h = *+1
        sbc     #$FF
        sta     cx32h

        ; t = ((x30 + x13 + x17) >> PJPG_DCT_SCALE_BITS) +128;
        clc
cx30l = *+1
        lda     #$FF
        adc     cx13l
        tay
cx30h = *+1
        lda     #$FF
        adc     cx13h
        tax
        tya

        clc
        adc     cx17l
        tay
        txa
        adc     cx17h

        INLINE_ASRYA7

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
cx31l = *+1
        lda     #$FF
cx32l = *+1
        sbc     #$FF
        tay
cx31h = *+1
        lda     #$FF
cx32h = *+1
        sbc     #$FF
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
cres3l = *+1
        adc     #$FF
        tay
        txa
cres3h = *+1
        adc     #$FF

        INLINE_ASRYA7

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


        ;x24 = res1 - imul_b2(x4
        ldy     cx4l
        ldx     cx4h
        jsr     _imul_b2
        sta     ptr1
        stx     ptr1+1

        sec
cres1l = *+1
        lda     #$FF
        sbc     ptr1
        tay
cres1h = *+1
        lda     #$FF
        sbc     ptr1+1
        tax

        ;x44 = res3 + x24;
        clc
        tya
        adc     cres3l
        tay
        txa
        adc     cres3h
        tax

        ; + x30
        clc
        tya
        adc     cx30l
        tya
        txa
        adc     cx30h
        tax
        ; -x13
        sec
        tya
        sbc     cx13l
        tay
        txa
        sbc     cx13h

        INLINE_ASRYA7

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
        clc
        lda     cx32l
        adc     cx31l
        tay
        lda     cx32h
        adc     cx31h
        tax

        ; t = ((x41 - res2) >> PJPG_DCT_SCALE_BITS) +128;
        tya
        sec
        sbc     cres2l
        tay
        txa
        sbc     cres2h

        INLINE_ASRYA7

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

;idctCols
idctCC: .res 1

;decodeNextMCU
componentID:.res 1
