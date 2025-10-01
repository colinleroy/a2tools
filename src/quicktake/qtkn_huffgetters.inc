
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

.macro GETDATAHUFF_REPVAL
        ldy     #0
        dec     _vbits        ; 1 bit
        bpl     :+
        jsr     _bitbuf_refill
:       asl     _bitbuf       ; Get bit into carry
        bcc     :++           ; If first bit is 0, value is 0

        iny
        dec     _vbits        ; Second bit
        bpl     :+
        jsr     _bitbuf_refill
:       asl     _bitbuf       ; Get bit into carry
        bcc     :+
        iny

:       ldx     huff_small_1,y
.endmacro

; Special for huff_data[2], where it's easy, 0 = 253 / 1 = 3
.macro GETDATAHUFF1BIT HUFF_TABLE
        lda     #253          ; huff code = 0

        dec     _vbits        ; How many remaining bits in the buffer?
        bpl     :+
        jsr     _bitbuf_refill
:       asl     _bitbuf       ; Get bit into carry
        bcc     :+
        lda     #3
:       tax
.endmacro

.macro GETDATAHUFF2BITS HUFF_TABLE
        lda     #0            ; huff code = 0
        ldx     _vbits
        dex                   ; First bit
        bpl     :+
        jsr     _bitbuf_refill

        asl     _bitbuf       ; Get two bits faster now we're full
        rol     a
        asl     _bitbuf
        rol     a
        dex                   ; consume second bit
        jmp     :+++

:       asl     _bitbuf       ; Get bit into carry
        rol     a             ; Put it into huff code

        dex                   ; Second bit
        bpl     :+
        jsr     _bitbuf_refill
:       asl     _bitbuf       ; Get bit into carry
        rol     a             ; Put it into huff code

:       tay                   ; And get value
        stx     _vbits
        ldx     HUFF_TABLE,y
.endmacro

.macro GETDATAHUFF2BITSPLUS HUFF_TABLE,filler_addr,rts_addr
        lda     #0            ; huff code = 0
        ldy     #1            ; numbits read = 1

        ldx     _vbits        ; How many remaining bits in the buffer?
        dex
        bpl     :+
        jsr     _bitbuf_refill
:
        asl     _bitbuf       ; Get bit into carry
        rol     a             ; Put it into huff code

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

.macro GETDATAHUFF_NREPEATS filler_addr,rts_addr
        GETDATAHUFF2BITSPLUS _huff_data+0*256,filler_addr,rts_addr
.endmacro

; No 0 value in these tables, no weird 0 prefix in codes,
; we can just try to load a value without counting bits
.macro GETDATAHUFF2BITSPLUS_SAFE HUFF_TABLE,filler_addr,rts_addr
        lda     #0            ; huff code = 0

        dec     _vbits        ; How many remaining bits in the buffer?
        bpl     :+
        jsr     _bitbuf_refill
:
        asl     _bitbuf       ; Get bit into carry
        rol     a             ; Put it into huff code

:       dec     _vbits        ; Remove from buffer remaining
        bmi     filler_addr   ; Go refill buffer if empty - comes back to rts_addr
rts_addr:
        asl     _bitbuf       ; Get bit into carry
        rol     a             ; Put it into huff code
        tay
        ldx     HUFF_TABLE+128,y
        beq    :-             ; Nope. Go read another bit
.endmacro

.macro DISCARDNDATAHUFF refiller_addr,rts_addr,table_label
        sty     num_discard

:       lda     #0            ; r = 0
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

.macro DISCARDNDATAHUFF_REPVAL
        ldx     _vbits
:       dex                   ; 1 bit
        bpl     :+
        jsr     _bitbuf_refill
:       asl     _bitbuf       ; Get bit into carry
        bcc     :++           ; If first bit is 0, value is 0, no other bit to pull

        dex                   ; Second bit
        bpl     :+
        jsr     _bitbuf_refill
:       asl     _bitbuf       ; Discard bit

:       dey
        bne     :----
        stx     _vbits
.endmacro

.macro DISCARDNBITS discardN_refill, discardN_rts
        ldy     #4
        ldx     _vbits
:       dex
        bmi     discardN_refill
discardN_rts:
        asl     _bitbuf       ; Discard bit
        dey
        bne     :-
        stx     _vbits
.endmacro

.macro DISCARD8BITS
        ldx     _vbits
        ldy     eight_min_x,x

        jsr     _bitbuf_refill
        ldx     #8
        cpy     #0
        beq     :++
:       dex                   ; No need to check for refill
        asl     _bitbuf       ; Discard bit
        dey
        bne     :-
:       stx     _vbits
.endmacro

.macro GETDATAHUFF_INIT data9_fill, data9_rts
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

.macro DISCARD4DATAHUFF_INIT
; Returns nothing, discards 4 5-bit tokens
        lda     #20
        sec
        sbc     _vbits            ; Once this byte is finished we have A bits to discard
        cmp     #16
        bcc     :+                ; Discard one or two full bytes?
        jsr     _bitbuf_refill    ; Discard a full byte,
        sec
        sbc     #8
:       jsr     _bitbuf_refill    ; Discard another full byte,

        jsr     _bitbuf_refill    ; Load the next one,

        ldx     #8                ; Note we didn't consume the bit, which refill expects us to
                                  ; so fix vbits from 7 to 8

        sec                       ; Remove last discarded 8 bits from remaining
        sbc     #8

        beq     :++               ; Any bits left to discard?
        tay

        lda     _bitbuf           ; Load buffer to shift faster
:       dex                       ; Discard the last bits, being sure we have
        asl                       ; enough bits in the bitbuffer not to reload
        dey
        bne     :-
        sta     _bitbuf
:       stx     _vbits

.endmacro
