        .export           _sierra_thumb_histogram
        .export           _sierra_load_thumb_data

        .import           _read, _lseek, _ifd, _buffer, _opt_histogram
        .import           pushax, pusha0, pusheax, tossub0ax

        .importzp         sreg

        .include          "../qt-thumbs.inc"
        .include          "stdio.inc"

.segment "SIERRA"

.proc _sierra_thumb_histogram
        ; Seek to start of data
        lda     _ifd
        jsr     pusha0

        jsr     push0         ; Whence is long
        lda     #(96*2)       ; Offset 96*2 to skip two lines
        jsr     pusha0
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

.proc _sierra_load_thumb_data
        and     #$01
        beq     :+
        rts

:       lda     _ifd
        jsr     pusha0
        lda     #<(_buffer + THUMBNAIL_BUFFER_OFFSET)
        ldx     #>(_buffer + THUMBNAIL_BUFFER_OFFSET)
        jsr     pushax
        lda     #<(THUMB_WIDTH)
        ldx     #>(THUMB_WIDTH)
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
