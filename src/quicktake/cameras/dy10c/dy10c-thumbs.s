        .export           _dy10c_thumb_histogram
        .export           _load_normal_thumb
        .export           _load_fine_thumb
        .export           _load_superfine_thumb

        .import           _err_buf, _thumb_buf, _thumb_len

        .import           _read, _lseek, _ifd, _buffer, _opt_histogram
        .import           pushax, pusha0, push0ax, tossub0ax
        .import           tosudiva0

        .importzp         _zp6, _zp8, _zp9, _zp10, _zp12, _zp13, tmp1, tmp2, sreg

        .include          "../qt-thumbs.inc"
        .include          "stdio.inc"
        .include          "fcntl.inc"

page            = _zp6        ; word
rem_bytes       = _zp8        ; byte
cur_byte        = _zp9        ; byte
curr_hist       = _zp10       ; word
prev_x          = _zp12       ; byte
cur_x           = _zp13       ; byte
.segment "DY10C"

.proc _dy10c_thumb_histogram
        lda     _ifd          ; go to end of file
        jsr     pusha0

        lda     #0
        tax
        jsr     push0ax
        lda     #<SEEK_END
        ldx     #>SEEK_END
        jsr     _lseek

        stx     _thumb_len    ; Note size.

        lda     _ifd          ; Rewind file
        jsr     pusha0

        lda     #0
        tax
        jsr     push0ax
        lda     #<SEEK_SET
        ldx     #>SEEK_SET
        jsr     _lseek

        ldx     #$00
next:                         ; Don't really do the histogram.
        txa
        sta     _opt_histogram,x
        inx
        bne     next
        rts
.endproc

.proc read_line
        pha
        lda     _ifd          ; Rewind file
        jsr     pusha0

        lda     #<(_buffer+256)
        ldx     #>(_buffer+256)
        jsr     pushax
        pla
        ldx     #$00
        jmp     _read
.endproc

.proc _load_normal_thumb
        and     #$3
        beq     :+
        rts

:       lda     #20
        jsr     read_line

        ldx     #19
        ldy     #159

:       lda     _buffer+256,x
        sta     _buffer+THUMBNAIL_BUFFER_OFFSET,y
        dey
        sta     _buffer+THUMBNAIL_BUFFER_OFFSET,y
        dey
        sta     _buffer+THUMBNAIL_BUFFER_OFFSET,y
        dey
        sta     _buffer+THUMBNAIL_BUFFER_OFFSET,y
        dey
        sta     _buffer+THUMBNAIL_BUFFER_OFFSET,y
        dey
        sta     _buffer+THUMBNAIL_BUFFER_OFFSET,y
        dey
        sta     _buffer+THUMBNAIL_BUFFER_OFFSET,y
        dey
        sta     _buffer+THUMBNAIL_BUFFER_OFFSET,y
        dey

        dex
        bne     :-
        rts
.endproc

.proc _load_fine_thumb
        and     #$1
        beq     :+
        rts

:       lda     #40
        jsr     read_line

        ldx     #39
        ldy     #159

:       lda     _buffer+256,x
        sta     _buffer+THUMBNAIL_BUFFER_OFFSET,y
        dey
        sta     _buffer+THUMBNAIL_BUFFER_OFFSET,y
        dey
        sta     _buffer+THUMBNAIL_BUFFER_OFFSET,y
        dey
        sta     _buffer+THUMBNAIL_BUFFER_OFFSET,y
        dey

        dex
        bne     :-
        rts
.endproc

.proc _load_superfine_thumb
        and     #$1
        beq     :+
        rts

:       lda     #80
        jsr     read_line

        ldx     #80
        ldy     #159

:       lda     _buffer+256-1,x
        asl
        asl
        asl
        asl
        asl
        asl
        sta     tmp1
        
        dex
        lda     _buffer+256-1,x
        lsr
        lsr
        ora     tmp1

        sta     _buffer+THUMBNAIL_BUFFER_OFFSET,y
        dey
        sta     _buffer+THUMBNAIL_BUFFER_OFFSET,y
        dey
        sta     _buffer+THUMBNAIL_BUFFER_OFFSET,y
        dey
        sta     _buffer+THUMBNAIL_BUFFER_OFFSET,y
        dey

        dex
        bne     :-
        rts
.endproc
