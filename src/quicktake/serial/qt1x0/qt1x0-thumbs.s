        .export           _qt1x0_thumb_histogram
        .export           _qt1x0_load_thumb_data

        .import           _is_qt100, _err_buf, _thumb_buf

        .import           _read, _lseek, _ifd, _buffer, _opt_histogram
        .import           pushax, pusha0, push0ax, tossub0ax
        .import           tosudiva0

        .importzp         _zp6, _zp8, _zp9, _zp10, tmp1, tmp2, sreg

        .include          "../qt-thumbs.inc"
        .include          "stdio.inc"
        .include          "fcntl.inc"

; _qt1x0_thumb_histogram
page            = _zp6        ; word
rem_bytes       = _zp8        ; byte
cur_byte        = _zp9        ; byte
curr_hist       = _zp10       ; word

.segment "QT1X0"

.proc _qt1x0_thumb_histogram
        lda     #0            ; Init values
        sta     rem_bytes     ; 256 bytes
        lda     #(8+1)        ; 8 pages
        sta     page
read_data:
        lda     _ifd          ; Read from thumbnail file
        jsr     pusha0

        lda     #<_buffer
        ldx     #>_buffer
        jsr     pushax
        lda     #<2048
        ldx     #>2048
        jsr     _read

        lda     #>_buffer     ; Set walker to buffer start
        sta     next_byte+2
        ldy     rem_bytes     ; How many bytes to read?
next_page:
next_byte:
        lda     _buffer,y     ; Read a byte (2 pixels)
        sta     tmp1
        asl                   ; First pixel
        asl
        asl
        asl
        tax
        inc     _err_buf,x
        bne     :+
        inc     _err_buf+256,x

:       lda     tmp1          ; Second pixel
        and     #$F0
        tax
        inc     _err_buf,x    ; histogram[x]++
        bne     :+
        inc     _err_buf+256,x

:       dey                   ; Anymore in this page ?
        bne     next_byte
        inc     next_byte+2
        dec     page          ; Any more pages ?
        bmi     done          ; if negative, we did the partial page
        bne     next_page     ; if not zero, more full pages

        ldy     #<(THUMBNAIL_SIZE-256-9) ; Last partial page
        sty     rem_bytes
        bne     read_data

done:
        ; We have counted values. Now equalize
        lda     _ifd          ; Rewind file
        jsr     pusha0

        lda     #0
        tax
        jsr     push0ax
        lda     #<SEEK_SET
        ldx     #>SEEK_SET
        jsr     _lseek

        lda     #$00
next_val:
        sta     cur_byte
        tax
        clc
        lda     _err_buf,x    ; curr_hist += histogram[x]
        adc     curr_hist
        sta     curr_hist
        tay
        lda     _err_buf+256,x
        adc     curr_hist+1
        sta     curr_hist+1

        tax                   ; curr_hist/20 (equ to *$F0 / 80 / 60)
        tya
        jsr     pushax
        lda     #20
        jsr     tosudiva0

        ldx     cur_byte      ; opt_histogram[x] = curr_hist/20
        sta     _opt_histogram,x

        txa                   ; x += $10 (as we only have $x0 values in thumbnails)
        clc
        adc     #$10
        bne     next_val
        rts
.endproc

.segment "QT1X0"

.proc _qt1x0_load_thumb_data
        ldy     _is_qt100
        beq     load_qt150_line
load_qt100_line:
        and     #$01          ; line & 1 ? if odd, return, to double lines
        beq     :+
        rts

:       lda     _ifd          ; Read from thumbnail file
        jsr     pusha0

        lda     #<(_buffer+THUMBNAIL_BUFFER_OFFSET)
        ldx     #>(_buffer+THUMBNAIL_BUFFER_OFFSET)
        jsr     pushax
        lda     #<(THUMB_WIDTH/2)
        ldx     #>(THUMB_WIDTH/2)
        jsr     _read

        ldy     #((THUMB_WIDTH/2)-1)
        ldx     #((THUMB_WIDTH/2)-1)*4

next_thumb_x:
        lda     _buffer+THUMBNAIL_BUFFER_OFFSET,y
        sta     tmp2          ; backup value for second pixel
        asl
        asl
        asl
        asl
        sta     _buffer+THUMBNAIL_BUFFER_OFFSET,x
        dex                   ; Store twice to double width
        sta     _buffer+THUMBNAIL_BUFFER_OFFSET,x

        lda     tmp2          ; Second pixel
        and     #$F0
        dex
        sta     _buffer+THUMBNAIL_BUFFER_OFFSET,x
        dex                   ; Store twice to double width
        sta     _buffer+THUMBNAIL_BUFFER_OFFSET,x
        dex

        dey
        bpl     next_thumb_x
        rts

load_qt150_line:
        and     #$03          ; Load two lines at once, and bump them to 4
        beq     qt150_first_line
        and     #$01
        beq     qt150_second_line
        rts

qt150_first_line:
        lda     _ifd          ; Read from thumbnail file
        jsr     pusha0

        lda     #<_thumb_buf
        ldx     #>_thumb_buf
        jsr     pushax
        lda     #<THUMB_WIDTH
        ldx     #>THUMB_WIDTH
        jsr     _read

        ldx     #$00
        ldy     #$00

next_expand_x:                ; expand nibbles to bytes
        lda     _thumb_buf,x
        sta     tmp1
        and     #$F0          ; high nibble
        sta     _buffer+THUMBNAIL_BUFFER_OFFSET,y
        iny

        lda     tmp1          ; low nibble
        asl
        asl
        asl
        asl
        sta     _buffer+THUMBNAIL_BUFFER_OFFSET,y
        iny

        inx
        cpx     #THUMB_WIDTH
        bcc     next_expand_x

        ; Reorder bytes across two rows: x/y, x+1/y,x/y+1 for 120 bytes, then x+1,y+1 for 40 bytes
        lda     #>_thumb_buf
        sta     out_high

        ldx     #$00          ; in pointer
        ldy     #$00          ; out pointer

reorder_120:
        lda     _buffer+THUMBNAIL_BUFFER_OFFSET,x ; X/Y
        sta     _thumb_buf,y

        inx
        iny
        lda     _buffer+THUMBNAIL_BUFFER_OFFSET,x ; X+1/Y
        sta     _thumb_buf,y

        inx
        lda     _buffer+THUMBNAIL_BUFFER_OFFSET,x ; X/Y+1
        sta     _thumb_buf+THUMB_WIDTH-1,y

        iny
        inx
        cpx     #(THUMB_WIDTH*3/2)
        bcc     reorder_120

reorder_40:
        iny
        lda     _buffer+THUMBNAIL_BUFFER_OFFSET,x ; X+1/Y
out_high:
        sta     _thumb_buf,y
        iny
        inx
        bne     check_reorder_bound
        inc     out_high                          ; Last pixels are on page 2
check_reorder_bound:
        cpx     #<(THUMB_WIDTH*2)
        bcc     reorder_40

        ; Done! finally, copy first line to second line for upscaling
qt150_second_line:
        ldx     #$00
        ldy     #$00
next_copy_x:
        lda     _thumb_buf,x
        sta     _buffer+THUMBNAIL_BUFFER_OFFSET,y
        iny
        sta     _buffer+THUMBNAIL_BUFFER_OFFSET,y
        iny
        inx
        cpx      #THUMB_WIDTH
        bcc      next_copy_x
        rts

.endproc
