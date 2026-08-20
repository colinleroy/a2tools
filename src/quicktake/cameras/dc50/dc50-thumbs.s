        .export           _dc50_thumb_histogram
        .export           _dc50_load_thumb_data

        .import           _read, _lseek, _ifd, _buffer, _opt_histogram
        .import           pushax, pusha0, pusheax, tossub0ax

        .importzp         sreg

        .include          "../qt-thumbs.inc"
        .include          "stdio.inc"

.segment "DC50"

.proc _dc50_thumb_histogram
        ; Seek to start of data
        lda     _ifd          ; Push twice, once for each lseek
        jsr     pusha0
        lda     _ifd
        jsr     pusha0

        lda     #$00          ; Offset 0 to end
        jsr     pusha0
        jsr     pusha0
        lda     #SEEK_END
        ldx     #0
        jsr     _lseek

        jsr     pusheax       ; Compute difference
        lda     #<(160*60)
        ldx     #>(160*60)
        jsr     tossub0ax

        jsr     pusheax       ; Push result to second lseek
        lda     #SEEK_SET
        ldx     #0
        jsr     _lseek

next:                         ; Don't really do the histogram.
        txa
        sta     _opt_histogram,x
        inx
        bne     next
        rts
.endproc

.proc _dc50_load_thumb_data
        and     #$01
        beq     :+
        rts

:       lda     _ifd
        jsr     pusha0
        lda     #<(_buffer + THUMBNAIL_BUFFER_OFFSET)
        ldx     #>(_buffer + THUMBNAIL_BUFFER_OFFSET)
        jsr     pushax
        lda     #<(THUMB_WIDTH*2)
        ldx     #>(THUMB_WIDTH*2)
        jsr     _read
    
        ldy #39
next:
        tya
        asl
        asl
        tax
        lda     _buffer+THUMBNAIL_BUFFER_OFFSET+1,x
        sta     _buffer+THUMBNAIL_BUFFER_OFFSET+2,x
        sta     _buffer+THUMBNAIL_BUFFER_OFFSET+3,x
        lda     _buffer+THUMBNAIL_BUFFER_OFFSET,x
        sta     _buffer+THUMBNAIL_BUFFER_OFFSET+1,x
        dey
        bpl     next
        rts
.endproc
