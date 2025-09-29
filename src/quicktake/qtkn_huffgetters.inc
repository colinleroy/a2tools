
.macro REFILLER filler_addr,rts_addr
filler_addr:
        jsr     _bitbuf_refill
        jmp     rts_addr
.endmacro

; Returns value in A
.macro GETCTRLHUFF filler_addr,rts_addr
        lda    #0             ; r = 0
        tay                   ; n = 0
        ldx     _vbits

read_bit:
        iny                   ; Read until valid code
        dex
        bmi     filler_addr
rts_addr:
        asl     _bitbuf
        rol     a
        sta     bitscheckc+1    ; Patch bitcheck address
                              ; cpy $nnnn,x is impossible so this is faster

bitscheckc:
huff_numc = *+2             ; Get num bits
        cpy     _huff_ctrl
        bne     read_bit
        stx     _vbits

        tax
huff_numc_h = *+2
        lda     _huff_ctrl+256,x
.endmacro

.macro GETDATAHUFF HUFF_TABLE,filler_addr,rts_addr
        lda     #0            ; huff code = 0
        tay                   ; numbits read = 0

        ldx     _vbits        ; How many remaining bits in the buffer?
:       iny                   ; Count read bit
        dex                   ; Remove from buffer remaining
        bmi     filler_addr   ; Go refill buffer if empty - comes back to data_cont
rts_addr:
        asl     _bitbuf       ; Get bit into carry
        rol     a             ; Put it into huff code

        sta     @table_patch+1; Patch bitcheck address right below (the array is
                              ; aligned, so that makes an ad-hoc tax / cpy $nnnn,x)

@table_patch:                 ; Is this code valid with this number of bits?
        cpy     HUFF_TABLE    ; (for example, 10 is not the same as 010!)
        bne    :-             ; Nope. Go read another bit

        stx    _vbits         ; Valid code. Save remaining
        tay                   ; And get value
        ldx     HUFF_TABLE+128,y
.endmacro

.macro DISCARDNDATAHUFF refiller_addr,rts_addr,table_label
        stx     num_discard

:
        lda     #0             ; r = 0
        tay                   ; n = 0

        ldx     _vbits
:       iny                   ; Read until valid code
        dex
        bmi     refiller_addr
rts_addr:
        asl     _bitbuf
        rol     a
        sta     table_label-1 ; Patch bitcheck address
                              ; cpy $nnnn,x is impossible so this is faster

table_label = *+2             ; Get num bits
        cpy     _huff_data
        bne    :-

        stx     _vbits

        dec     num_discard
        bne     :--
.endmacro

.macro GETDATAHUFF9 data9_fill, data9_rts
        lda     #0             ; Read and consume 5 bits
        ldy     #5
        ldx     _vbits
:       dex
        bmi     data9_fill
data9_rts:
        asl     _bitbuf
        rol     a
        dey
        bne     :-

        stx     _vbits
        tax
        lda    _ushiftl3p4,x
.endmacro

.macro DISCARD4DATAHUFF9 data9_discard_fill, data9_discard_rts
; Returns nothing, discards 4 5-bit tokens
; TODO: don't shift all bits, skip full bytes
; Not too urgent as function is called ~30 times
; and accounts for less than 20k cycles total
        ldy     #20
        ldx     _vbits
:       dex
        bmi     data9_discard_fill
data9_discard_rts:
        asl     _bitbuf
        dey
        bne     :-
        stx     _vbits
.endmacro
