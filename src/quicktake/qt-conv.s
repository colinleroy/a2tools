        .export         _write_raw
        .import         _histogram_low, _histogram_high

        .import         _write, _ofd
        .import         _special_x_orig_offset, _orig_x_offset
        .import         _orig_y_table_l, _orig_y_table_h
        .import         _output_write_len
        .import         _scaled_band_height, _last_band, _last_band_crop
        .import         _raw_image

        .import         pusha0, pushax
        .importzp       tmp1, _zp4
y_ptr = _zp4

.proc _write_raw
        ldy     _last_band_crop   ; Is there cropping?
        beq     full_band

        cmp     _last_band        ; Are we on last band?
        bne     full_band
        cpx     _last_band+1
        bne     full_band

        lda     _last_band_crop   ; Register Y stop
        sta      y_end+1

        ; output_write_len -= (scaled_band_height - last_band_crop) * FILE_WIDTH;
        ; (FILE_WIDTH = 256 so shift 8)
        lda      _scaled_band_height
        sec
        sbc      _last_band_crop
        sta      tmp1
        sec
        lda      _output_write_len+1
        sbc      tmp1
        sta      _output_write_len+1
        jmp      no_crop

full_band:
        lda     _scaled_band_height
        sta     y_end+1

no_crop:
        lda     #>_raw_image
        sta     dst_ptr+2

        clc

        ldy     #$00
next_y:
        lda     _orig_y_table_l,y
        sta     cur_orig_y_addr+1
        ldx     _orig_y_table_h,y
        stx     cur_orig_y_addr+2

        iny
        sty     y_ptr

        ldy     #$00
        ldx     _orig_x_offset    ; Preload the first X offset, and
        jmp     cur_orig_y_addr   ; skip the first page increment
next_x:
        ldx     _orig_x_offset,y  ; if (*cur_orig_x == 0) cur+=256
        bne     cur_orig_y_addr
        ldx     _special_x_orig_offset,y
        inc     cur_orig_y_addr+2

cur_orig_y_addr:
        lda     $FFFF,x           ; patched, get current pixel
dst_ptr:
        sta     _raw_image,y      ; Store scaled

        tax                       ; update histogram
        inc     _histogram_low,x
        bne     :+
        inc     _histogram_high,x

:       iny
        bne     next_x
        inc     dst_ptr+2         ; Next output page

        ldy     y_ptr
y_end:  cpy     #$FF              ; Patched
        bcc     next_y

        lda     _ofd
        jsr     pusha0
        lda     #<_raw_image
        ldx     #>_raw_image
        jsr     pushax
        lda     _output_write_len
        ldx     _output_write_len+1
        jmp     _write
.endproc

.segment "BSS"
