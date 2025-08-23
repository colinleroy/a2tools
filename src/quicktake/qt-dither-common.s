        .export         _load_normal_data
        .export         _update_progress_bar
        .import         _load_thumb_data

        .import         _progress_bar, _scrw
        .import         _file_height, _file_width
        .import         _hgr_baseaddr_l, _hgr_baseaddr_h
        .import         _first_byte_idx, _resize
        .import         _x_offset, _crop_pos

        .import         _bayer_map, _bayer_map_x, _bayer_map_y
        .import         _end_bayer_map_x, _end_bayer_map_y
        .import         _err_buf
        .import         _opt_histogram
        .import         _cur_hgr_row
        .import         _is_thumb, _load_thumbnail_data
        .import         _prev_scaled_dx, _prev_scaled_dy, _angle
        .import         _brighten, _dither_alg
        .import         _kbhit, _cgetc
        .import         _read_buf, _cur_buf_page, _buffer

        .export         img_y

        .import         pushax, pusha0, _read, _ifd, subysp

        .importzp       ptr1, tmp1, tmp2, c_sp, sreg, ptr2
        .importzp       _zp2p, _zp4, _zp5, _zp6, _zp7, _zp8, _zp9, _zp10, _zp11, _zp12ip

        .include "apple2.inc"

img_y                 = _zp5

_load_normal_data:
        lda     img_y
        and     #7
        beq     read_buffer
        inc     _cur_buf_page+1
        rts

read_buffer:
        lda     _ifd
        ldx     #0
        jsr     pushax
        lda     #<_buffer
        ldx     #>_buffer
        jsr     pushax
        lda     #<2048      ; BUFFER_SIZE FIXME
        ldx     #>2048
        jsr     _read
        lda     #>_buffer
        sta     _cur_buf_page+1
        rts

_update_progress_bar:
        ldy     #10
        jsr     subysp
        lda     #$FF

        dey                              ; -1,
        sta     (c_sp),y
        dey
        sta     (c_sp),y

        dey                              ; -1,
        sta     (c_sp),y
        dey
        sta     (c_sp),y

        dey                              ; scrw,
        lda     #$0
        sta     (c_sp),y
        dey
        lda     _scrw
        sta     (c_sp),y

        dey                              ; pgbar_state (long)
        lda     #0
        sta     (c_sp),y
        dey
        sta     (c_sp),y
        dey
        sta     (c_sp),y
        dey
        lda     #192
        sec
        sbc     img_y
        sta     (c_sp),y

        lda     #$00
        sta     sreg+1                   ; height (long)
        sta     sreg
        lda     _file_height
        ldx     _file_height+1
        jmp     _progress_bar
