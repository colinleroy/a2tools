
        .importzp   tmp1, tmp2, tmp3, tmp4, ptr1, ptr2, ptr3, ptr4, _prev_ram_irq_vector, _prev_rom_irq_vector, c_sp
        .importzp   _zp2, _zp3, _zp4, _zp5, _zp6, _zp8, _zp7, _zp9, _zp10, _zp11, _zp12, _zp13
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
        .export     _getByteNoFF, _setFFCheck
; ZP vars. Mind that qt-conv uses some too
_gBitBuf       = _zp2       ; byte, used everywhere
n              = _zp3       ; byte, used in getBits
rowMCUflags    = _zp4       ; used in _decodeNextMCU
bbHigh         = _zp5       ; byte, used in huffDecode and getBits
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
        cpx     #9
        bcs     large_n
small_n:
        lda     #0
:       INLINE_GETBIT 1
        rol     a
        dex
        bne     :-
        ; X is zero
        rts

large_n:
        ldy     n_min_eight,x ; How much more than 8?
        sty     last_few+1

        lda     #0            ; Init result
        sta     bbHigh

        ldx     #8            ; Get the first eight
:       INLINE_GETBIT 1
        rol     a
        dex
        bne     :-

last_few:
        ldx     #$FF          ; Get the last (n-8)
:       INLINE_GETBIT 1
        rol     a
        rol     bbHigh
        dex
        bne     :-

        ldx     bbHigh
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
        rol             ; Now A = 0 or 1 depending on bit

        ldx     #$00
nextLoopS:
        ldy     TABLE+hufftable_t::mGetMore,x
        bne     incrementS

        cmp     TABLE+hufftable_t::mMaxCode_l,x
        bcc     loopDone
incrementS:
        INLINE_GETBIT 1
        rol     a
        inx
        cpx     #8
        bne     nextLoopS
        jmp     decodeLong

loopDone:
        adc     TABLE+hufftable_t::mValPtr,x
        tax                     ; Get index

        lda     VAL,x
        rts

decodeLong:
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
        bcc     loopDone
incrementL:
        INLINE_GETBIT 1
        rol     a
        rol     bbHigh
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

        lda    _gCoeffBuf+3,y
        bne    full_idct_rows
        lda    _gCoeffBuf+5,y
        bne    full_idct_rows
        lda    _gCoeffBuf+7,y
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

rx5l  = ptr1
rx5h  = ptr1+1
rx7l = ptr2
rx7h = ptr2+1
rx4l = ptr3
rx4h = ptr3+1
rx13l = ptr4
rx13h = ptr4+1

        ; x7 and x4
        clc
        lda    _gCoeffBuf+6,y
        sta    rx7l
        eor    #$FF
        adc    #1
        sta    rx4l
        lda    _gCoeffBuf+7,y
        sta    rx7h
        eor    #$FF
        adc    #0
        sta    rx4h

        ; x5
        clc
        lda    _gCoeffBuf+2,y
        sta    rx5l
        lda    _gCoeffBuf+3,y
        sta    rx5h

        ; x30
        clc
        lda    _gCoeffBuf,y
        adc    _gCoeffBuf+8,y
        sta    rx30l
        lda    _gCoeffBuf+1,y
        adc    _gCoeffBuf+9,y
        sta    rx30h

        ; x31
        sec
        lda    _gCoeffBuf,y
        sbc    _gCoeffBuf+8,y
        sta    rx31l
        lda    _gCoeffBuf+1,y
        sbc    _gCoeffBuf+9,y
        sta    rx31h

        ; x13
        lda    _gCoeffBuf+4,y
        sta    rx13l
        lda    _gCoeffBuf+5,y
        sta    rx13h

        ; x17 = x5+x7
        clc
        lda    rx5l
        adc    rx7l
        sta    rx17l
        tay
        lda    rx5h
        adc    rx7h
        sta    rx17h
        tax

        ;*(rowSrc) = x30 + x13 + x17;
        clc
        tya
        adc    rx13l
        tay
        txa
        adc    rx13h
        tax
        tya

        clc
        adc    rx30l
        ldy    tmp3
        sta    _gCoeffBuf,y
        txa
        adc    rx30h
        sta    _gCoeffBuf+1,y

        ; x32 = imul_b1_b3(x13) - x13;
        ldy    rx13l
        ldx    rx13h
        jsr    _imul_b1_b3
        sec
        sbc    rx13l
        sta    rx32l
        txa
        sbc    rx13h
        sta    rx32h

        ; res1 = imul_b5(x4 - x5);
        sec
        lda    rx4l
        sbc    rx5l
        tay
        lda    rx4h
        sbc    rx5h
        tax
        jsr    _imul_b5
        sta    rres1l
        stx    rres1h

        ; res2 = imul_b4(x5) - res1 - x17;
        ldy    rx5l
        ldx    rx5h
        jsr    _imul_b4
        sec                     ; -res1
        sbc    rres1l
        tay
        txa
        sbc    rres1h
        tax

        tya
        sec                     ; -x17
rx17l = *+1
        sbc    #$FF
        sta    rres2l
        txa
rx17h = *+1
        sbc    #$FF
        sta    rres2h

        ; x15 = x5 - x7;
        sec
        lda    rx5l
        sbc    rx7l
        tay
        lda    rx5h
        sbc    rx7h
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
rx31l = *+1
        adc    #$FF
        tay
        txa
rx31h = *+1
        adc    #$FF
        tax

        tya
        sec
rx32l = *+1
        sbc    #$FF
        ldy     tmp3          ; and restore
        sta    _gCoeffBuf+4,y
        txa
rx32h = *+1
        sbc    #$FF
        sta    _gCoeffBuf+5,y

        ; x24 = res1 - imul_b2(x4);
        ldy    rx4l
        ldx    rx4h
        jsr    _imul_b2
        sta    tmp1
        stx    tmp2

        sec
rres1l = *+1
        lda    #$FF
        sbc    tmp1
        tay
rres1h = *+1
        lda    #$FF
        sbc    tmp2
        tax

        ; *(rowSrc_4) = x24 + x30 + res3 - x13;
        clc
        tya
rx30l = *+1
        adc    #$FF
        tay
        txa
rx30h = *+1
        adc    #$FF
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
        sbc    rx13l
        ldy    tmp3
        sta    _gCoeffBuf+8,y
        txa
        sbc    rx13h
        sta    _gCoeffBuf+9,y

        ; *(rowSrc_6) = x31 + x32 - res2;
        clc
        lda    rx31l
        adc    rx32l
        sta    tmp1
        lda    rx31h
        adc    rx32h
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
        lda     _gCoeffBuf+16,y
        bne     full_idct_cols
        lda     _gCoeffBuf+32,y
        bne     full_idct_cols
        lda     _gCoeffBuf+48,y
        bne     full_idct_cols
        lda     _gCoeffBuf+17,y
        bne     full_idct_cols
        lda     _gCoeffBuf+33,y
        bne     full_idct_cols
        lda     _gCoeffBuf+49,y
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

cx4l  = ptr1
cx4h  = ptr1+1
cx30l = ptr2
cx30h = ptr2+1
cx5l  = ptr3
cx5h  = ptr3+1
cx12l = ptr4
cx12h = ptr4+1

        sty     tmp3          ; Backup before b5/b2/b4/b1_b3 mults

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

        ; cx4 and cx7
        lda     _gCoeffBuf+48,y
        sta     cx7l
        clc
        eor     #$FF
        adc     #1
        sta     cx4l
        lda     _gCoeffBuf+49,y
        sta     cx7h
        eor     #$FF
        adc     #0
        sta     cx4h

        ;x17 = x5 + x7;
        clc
        lda     cx5l
cx7l = *+1
        adc     #$FF
        sta     cx17l
        lda     cx5h
cx7h = *+1
        adc     #$FF
        sta     cx17h

        ;res1 = imul_b5(x4 - x5)
        sec
        lda     cx4l
        sbc     cx5l
        tay
        lda     cx4h
        sbc     cx5h
        tax
        jsr     _imul_b5
        sta     cres1l
        stx     cres1h

        ;res2 = imul_b4(x5) - res1 - x17;
        ldy     cx5l
        ldx     cx5h
        jsr     _imul_b4

        sec
        sbc     cres1l
        tay
        txa
        sbc     cres1h
        tax

        sec
        tya
cx17l = *+1
        sbc     #$FF
        sta     cres2l
        txa
cx17h = *+1
        sbc     #$FF
        sta     cres2h

        ;res3 = imul_b1_b3(x5 - x7) - res2;
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

        ; t = ((x30 + x12 + x17) >> PJPG_DCT_SCALE_BITS) +128;
        clc
        lda     cx30l
        adc     cx12l
        tay
        lda     cx30h
        adc     cx12h
        tax

        clc
        tya
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

        ; cx32 = imul_b1_b3(cx12) - cx12;
        ldy     cx12l
        ldx     cx12h
        jsr     _imul_b1_b3
        sec
        sbc     cx12l
        sta     cx32l
        tay
        txa
        sbc     cx12h
        sta     cx32h
        tax

        ; t = ((x30 + x32 - res2) >> PJPG_DCT_SCALE_BITS) +128;
        clc
        tya
        adc     cx30l
        tay
        txa
        adc     cx30h
        tax
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

        ;x42 = x30 - x32;
        sec
        lda     cx30l
cx32l = *+1
        sbc     #$FF
        tay
        lda     cx30h
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

        ;x24 = res1 - imul_b2(x4)
        ldy     cx4l
        ldx     cx4h
        jsr     _imul_b2
        sta     tmp1
        stx     tmp2

        sec
cres1l = *+1
        lda     #$FF
        sbc     tmp1
        tay
cres1h = *+1
        lda     #$FF
        sbc     tmp2
        tax

        ;+ res3
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
        ; -x12
        sec
        tya
        sbc     cx12l
        tay
        txa
        sbc     cx12h

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

cont_idct_cols:
        dec     idctCC
        beq     idctColDone
        iny
        iny
        jmp     nextCol

idctColDone:
        rts

        GET_BITS_SET_FF_ON
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

        lda     #0
        sta     rowMCUflags
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
        stx     cur_ZAG_coeff ; Store cur_ZAG_coeff after looping in finishZAG
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
