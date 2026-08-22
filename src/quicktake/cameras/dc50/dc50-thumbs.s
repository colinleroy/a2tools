        .export           _dc50_thumb_histogram
        .export           _dc50_load_thumb_data

        .import           _read, _lseek, _ifd, _buffer, _opt_histogram
        .import           pushax, pusha0, push0

        .importzp         sreg

        .include          "../qt-thumbs.inc"
        .include          "stdio.inc"

.segment "DC50"

.proc _dc50_thumb_histogram
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

.proc _dc50_load_thumb_data
        and     #$03
        beq     :+
        rts

:       lda     _ifd
        jsr     pusha0
        lda     #<(_buffer + 256)
        ldx     #>(_buffer + 256)
        jsr     pushax
        lda     #<(96)
        ldx     #>(96)
        jsr     _read
    
        ldx     #8
        ldy     #0
next:
        lda     _buffer+256,x
        and     #$F0
        sta     _buffer+THUMBNAIL_BUFFER_OFFSET,y
        sta     _buffer+THUMBNAIL_BUFFER_OFFSET+1,y
        sta     _buffer+THUMBNAIL_BUFFER_OFFSET+2,y
        sta     _buffer+THUMBNAIL_BUFFER_OFFSET+3,y

        lda     _buffer+256+1,x
        asl
        asl
        asl
        asl
        sta     _buffer+THUMBNAIL_BUFFER_OFFSET+4,y
        sta     _buffer+THUMBNAIL_BUFFER_OFFSET+5,y
        sta     _buffer+THUMBNAIL_BUFFER_OFFSET+6,y
        sta     _buffer+THUMBNAIL_BUFFER_OFFSET+7,y

        clc
        txa
        adc     #3
        tax
        tya
        adc     #8
        tay
        cpy     #(THUMB_WIDTH*2)
        bcc     next

        rts
.endproc
