        .export         _setup_angle_0
        .export         _setup_angle_90
        .export         _setup_angle_180
        .export         _setup_angle_270
        .export         _update_progress_bar
        .export         _do_dither
        .export         _load_normal_data
        .import         _load_thumb_data

        .import         _progress_bar, _scrw
        .import         _file_height, _file_width
        .import         _centered_div7_table, _div7_table
        .import         _centered_mod7_table, _mod7_table
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

        .import         pushax, pusha0, _fread, _ifp, subysp

        .importzp       ptr1, tmp1, tmp2, c_sp, sreg, ptr2
        .importzp       _zp2p, _zp4, _zp5, _zp6, _zp7, _zp8, _zp9, _zp10, _zp11, _zp12ip

        .include "apple2.inc"

thumb_buf_ptr         = _zp2p
img_x                 = _zp4
img_y                 = _zp5
pixel_val             = _zp6
pixel_mask            = _zp7
opt_val               = _zp8
err2                  = _zp9
hgr_byte              = _zp10
cur_hgr_line          = _zp11
hgr_line              = _zp12ip

DITHER_NONE       = 2
DITHER_BAYER      = 1
DITHER_SIERRA     = 0
DITHER_THRESHOLD  = $80
DEFAULT_BRIGHTEN  = 0

CENTER_OFFSET       = 0
CENTER_OFFSET_THUMB = 60-12

sierra_safe_err_buf = _err_buf+1

.assert <_centered_div7_table = $00, error ; We count on the table being aligned with idx 12 on a page border
.assert <_centered_mod7_table = $00, error ; We count on the table being aligned with idx 12 on a page border


; defaults:
FIRST_PIXEL_HANDLER = dither_sierra
LINE_DITHER_SETUP   = prepare_dither_sierra

; dithering macros.
; They must end with jmp next_pixel, unless
; it's the last one in the loop.
.macro BAYER_DITHER_PIXEL BRANCH_TO
        ldy     #$00
        ldx     _bayer_map_x
        lda     _bayer_map,x
        bpl     positive_b
        dey
positive_b:
        adc     opt_val
        tax                   ; Backup low byte
        tya
        adc     #0

        bmi     black_pix_bayer
        bne     white_pix_bayer
        cpx     #<DITHER_THRESHOLD
        bcc     black_pix_bayer

white_pix_bayer:
        ldy     hgr_byte
        lda     pixel_val
        ora     pixel_mask
        sta     pixel_val

black_pix_bayer:
        ; Advance Bayer X
        ldx     _bayer_map_x
        inx
        cpx     _end_bayer_map_x
        beq     reset_bayer_x
        stx     _bayer_map_x
        jmp     BRANCH_TO

reset_bayer_x:
        ldx     _bayer_map_y
        stx     _bayer_map_x
        jmp     BRANCH_TO
.endmacro

.macro NO_DITHER_PIXEL BRANCH_TO
        lda     opt_val
        cmp     #<DITHER_THRESHOLD
        bcc     :+
        lda     pixel_val
        ora     pixel_mask
        sta     pixel_val
:       jmp     BRANCH_TO
.endmacro

.macro SIERRA_DITHER_PIXEL
        ; Add the two errors (the one from previous pixel and the one
        ; from previous line). As they're max 128/2 and 128/4, don't bother
        ; about overflows.
        ldx     #$00
        ldy     img_x
        lda     sierra_safe_err_buf,y
        adc     err2
        bpl     err_pos
        dex

err_pos:
      ; Add current pixel value
        clc
        adc     opt_val
        bcc     :+
        cpx     #$00
        bpl     white_pix     ; High byte not zero? - don't check for neg here, can't happen, it's either 0 or 1
        jmp     check_low
:       cpx     #$80          ; is high byte negative? (don't check for positive here, can't happen, it's either $FF or 0)
        bcs     forward_err

; Must check low byte
check_low:
        cmp     #<DITHER_THRESHOLD
        bcc     forward_err
white_pix:
        tax                   ; Backup low byte
        ldy     hgr_byte
        lda     pixel_val
        ora     pixel_mask
        sta     pixel_val
        txa                   ; Restore low byte

        cmp     #$80          ; Keep low byte sign for >> (no need to do it where coming from low byte check path, DITHER_THRESHOLD is $80)
forward_err:
        ror     a
        sta     err2
        cmp     #$80
        ror     a

        ; *(cur_err_x_yplus1+img_x)   = err1;
        ldy     img_x
        sta     sierra_safe_err_buf,y

sierra_err_forward:
        iny                   ; Patched with iny at setup, if xdir < 0
        clc                   ; May be set by ror
        adc     sierra_safe_err_buf,y
        sta     sierra_safe_err_buf,y
.endmacro

; Load a byte from the file. Must end with byte in A.
.macro LOAD_IMAGE_BYTE
buf_ptr_load:
        ldy     $FFFF         ; Patched with buf_ptr address
        ; opt_val = opt_histogram[*buf_ptr];
        lda     _opt_histogram,y
        sta     opt_val
.endmacro

; Prepare line's dithering
.macro LINE_PREPARE_DITHER
line_prepare_dither_handler:
        jmp     LINE_DITHER_SETUP   ; PATCHED to prepare_dither_{bayer,sierra,none}

prepare_dither_bayer:
        lda     _bayer_map_y
        sta     _bayer_map_x
        clc
        adc     #8
        sta     _end_bayer_map_x

prepare_dither_sierra:
        ; reset err2
        lda     #$00
        sta     err2

prepare_dither_none:          ; Nothing to do here.
.endmacro

.macro HGR_RESET_PIXEL_MASK
hgr_reset_mask = *+1
        lda     #$01                  ; Set init value     ($40 or $01)
        sta     pixel_mask
.endmacro

.macro HGR_STORE_PIXEL
        lda     pixel_val
        sta     (hgr_line),y
        lda     #0
        sta     pixel_val             ; Reset pixel val
.endmacro

.macro HGR_SET_START_MASK
        lda     hgr_start_mask
        sta     pixel_mask
.endmacro

.macro HGR_SET_START_BYTE
        lda     hgr_start_byte
        sta     hgr_byte
.endmacro

; Line = Y
.macro HGR_SET_LINE_POINTER
        lda     _hgr_baseaddr_l,y
        sta     hgr_line
        lda     _hgr_baseaddr_h,y
        sta     hgr_line+1
        sty     cur_hgr_line
        HGR_SET_START_BYTE
        HGR_SET_START_MASK
.endmacro

.macro BAYER_INIT
        lda     #0
        sta     _bayer_map_y
        lda     #64
        sta     _end_bayer_map_y
.endmacro

.macro BAYER_START_LINE
.endmacro

.macro BAYER_END_LINE
        ; Advance Bayer Y
        lda     _bayer_map_y
        clc
        adc     #8
        cmp     _end_bayer_map_y
        bne     store_bayer_y

        lda     #0
store_bayer_y:
        sta     _bayer_map_y
.endmacro


.macro BUFFER_INIT
        ldx     #>(_buffer)
        stx     _cur_buf_page+1
        stx     thumb_buf_ptr+1
        lda     #<(_buffer)
        sta     _cur_buf_page
        sta     thumb_buf_ptr
.endmacro

.macro LOAD_DATA
        lda     _is_thumb
        beq     load_normal
        jsr     _load_thumbnail_data
        jmp     :+

load_normal:
        jsr     _load_normal_data
:
        ldx     _cur_buf_page+1
        stx     buf_ptr_load+2
        lda     _cur_buf_page
        sta     buf_ptr_load+1
.endmacro

.macro ADVANCE_IMAGE_SLOW BRANCH_TO
.scope
        inc     buf_ptr_load+1
        beq     inc_buf_ptr_high
        jmp     BRANCH_TO

inc_buf_ptr_high:
        inc     buf_ptr_load+2
        jmp     BRANCH_TO
.endscope
.endmacro

.macro ADVANCE_IMAGE_FAST BRANCH_TO
.scope
        inc     buf_ptr_load+1
        bne     BRANCH_TO

inc_buf_ptr_high:
        inc     buf_ptr_load+2
        jmp     BRANCH_TO
.endscope
.endmacro

.macro PROGRESS_BAR_UPDATE BRANCH_TO
        dec     pgbar_update
        bne     BRANCH_TO

        lda     #16
        sta     pgbar_update

        jsr     _update_progress_bar
        jsr     _kbhit
        beq     BRANCH_TO
        jsr     _cgetc
        cmp     #$1B          ; Esc
        bne     BRANCH_TO
        rts
.endmacro

_do_dither:
        lda     #16
        sta     pgbar_update

        ; Image lines counter
        lda     _file_height
        sta     img_y
        ; Image rows counter
        lda     _file_width
        sta     img_x

        ; Safe init of scaling
        lda     #100
        sta     rotated_x_hgr_line
        sta     rotated_y_hgr_column

dither_setup_start:
        ldx     #CENTER_OFFSET
        lda     _is_thumb
        beq     center_done
        ldx     #CENTER_OFFSET_THUMB

center_done:
        stx     x_center_offset

        jsr     patch_dither_branches

        lda     _angle+1
        beq     :+
        jmp     angle270
:       lda     _angle
        cmp     #0
        beq     angle0
        cmp     #90
        beq     angle90
        jmp     angle180
angle0:
        jsr     _setup_angle_0
        jmp     finish_patches

angle90:
        jsr     _setup_angle_90
        jmp     finish_patches

angle180:
        jsr     _setup_angle_180
        jmp     finish_patches

angle270:
        jsr     _setup_angle_270

finish_patches:

        ; Line loop setup
        BUFFER_INIT
        BAYER_INIT

dither_setup_line_start_landscape:
        ldy     cur_hgr_line
        HGR_SET_LINE_POINTER

dither_setup_line_start_portrait:
        LINE_PREPARE_DITHER

dither_setup_line_start_portrait_skip_prec:
        LOAD_DATA

pixel_handler:
        LOAD_IMAGE_BYTE

pixel_handler_first_step:
        jmp     FIRST_PIXEL_HANDLER ; Will get patched with the most 
                                    ; direct thing possible
                                    ; (do_brighten, dither_bayer, ...)

; Sierra inlined to spare a jump. No dither and bayer are out of the
; main code path.
dither_sierra:
        SIERRA_DITHER_PIXEL

next_pixel:
        dec     img_x
        beq     line_done

        ; Advance to next HGR pixel after incrementing image X.
        ; The first three bytes are patched according to the rotation of the image.
img_x_to_hgr:
        asl     pixel_mask            ; Update pixel mask  (lsr or asl, or bit if inverted coords)
        bpl     advance_image_byte    ; Check if byte done (bne or bpl)
        HGR_RESET_PIXEL_MASK
        ldy     hgr_byte
        HGR_STORE_PIXEL
hgr_byte_shift:
        iny
        sty     hgr_byte              ; Update hgr byte idx(dec or inc)

advance_image_byte:
        ADVANCE_IMAGE_FAST pixel_handler

line_done:
        PROGRESS_BAR_UPDATE line_wrapup

line_wrapup:
        jmp $FFFF             ; PATCHED with either advance_bayer_y or next_line

advance_bayer_y:
        BAYER_END_LINE

next_line:
        dec     img_y
        beq     all_done

        ldy     hgr_byte
        HGR_STORE_PIXEL       ; Store the last pixel of the line

        ; Advance to the next HGR line after incrementing image Y.
        ; The first three bytes are patched according to the rotation of the image
img_y_to_hgr:
        ldy     cur_hgr_line
        iny
        HGR_SET_LINE_POINTER

        jmp     dither_setup_line_start_landscape
all_done:
        rts

dither_none:
        NO_DITHER_PIXEL     next_pixel
dither_bayer:
        BAYER_DITHER_PIXEL  next_pixel

; Brightening, out of the main code path
do_brighten:
        ldx     #0
        lda     _brighten
        bpl     brighten_pos
        dex
brighten_pos:
        adc     opt_val
        bcc     :+
        inx
:       cpx     #0
        beq     store_opt
        bpl     pos_opt
        lda     #0
        jmp     store_opt

pos_opt:
        lda     #$FF
store_opt:
        sta     opt_val
dither_after_brighten:
        jmp     $FFFF

_load_normal_data:
        lda     img_y
        and     #7
        beq     read_buffer
        inc     _cur_buf_page+1
        rts

read_buffer:
        lda     #<_buffer
        ldx     #>_buffer
        jsr     pushax
        lda     #1
        jsr     pusha0
        lda     #<2048      ; BUFFER_SIZE FIXME
        ldx     #>2048
        jsr     pushax
        lda     _ifp
        ldx     _ifp+1
        jsr     _fread
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


; Rotation setup. Will patch 
; - img_x_to_hgr
; - img_y_to_hgr
; - incs, decs, etc

C_ASL_ZP = $06
C_LSR_ZP = $46
C_BPL    = $10
C_BNE    = $D0
C_INC_ZP = $E6
C_DEC_ZP = $C6
C_LDY_ZP = $A4
C_INY    = $C8
C_DEY    = $88
C_JMP    = $4C

.macro MULT_BY_3_DIV_BY_4
.scope
        sta     tmp2
        ldx     #0
        stx     tmp1
        asl     a
        rol     tmp1
        adc     tmp2
        bcc     :+
        inc     tmp1
:       lsr     tmp1
        ror     a
        lsr     tmp1
        ror     a             ; We have our result
.endscope
.endmacro

_setup_angle_0:
        ; Set start constants
        ; Center vertically (for thumbs)
        lda     #192
        sec
        sbc     img_y
        clc
        lsr
        sta     cur_hgr_line

        ; Center horizontally
        ldx     x_center_offset
        lda     _centered_div7_table,x
        sta     hgr_start_byte
        lda     _centered_mod7_table,x
        sta     hgr_start_mask

        lda     #$01
        sta     hgr_reset_mask

        ; Patch img_x_to_hgr
        lda     #C_ASL_ZP
        sta     img_x_to_hgr
        lda     #pixel_mask
        sta     img_x_to_hgr+1
        lda     #C_BPL
        sta     img_x_to_hgr+2
        lda     #C_INY
        sta     hgr_byte_shift

        ; Patch img_y_to_hgr
        lda     #C_LDY_ZP
        sta     img_y_to_hgr
        lda     #cur_hgr_line
        sta     img_y_to_hgr+1
        lda     #C_INY
        sta     img_y_to_hgr+2

        rts

_setup_angle_90:
        ; Set start constants for clockwise rotation
        lda     #0
        sta     cur_hgr_line

        ldx     _resize
        bne     resize_90_coords
no_resize_90_coords:
        ; The 256x192 will be 192x192, cropped.
        ; first byte of the image at 236,0
        ; last byte of first line at 236, 191
        ; last byte of image at 32, 191
        ; Center horizontally
        ldx     #(280-((280-192)/2))
        lda     _div7_table,x
        sta     hgr_start_byte
        lda     _mod7_table,x
        sta     hgr_start_mask

        ; Insert the inverter before the first step (dithering or brightening)
        lda     pixel_handler_first_step+1
        sta     invert_line_done+1
        lda     pixel_handler_first_step+2
        sta     invert_line_done+2
        lda     #<invert_rotated_line
        sta     pixel_handler_first_step+1
        lda     #>invert_rotated_line
        sta     pixel_handler_first_step+2

        ldx     _crop_pos
        lda     crop_shift_90,x
        sta     x_shifter
        lda     crop_low_90,x
        sta     x_invert_low_bound
        lda     crop_high_90,x
        sta     x_invert_high_bound

        lda     #((280-192)/2)
        sta     y_shifter

        ; Patch img_y_to_hgr
        lda     #C_JMP
        sta     img_y_to_hgr
        lda     #<get_rotated_hgr_byte_from_img_y
        sta     img_y_to_hgr+1
        lda     #>get_rotated_hgr_byte_from_img_y
        sta     img_y_to_hgr+2

        jmp     end_90_setup
resize_90_coords:
        ; The 256x192 will be 144x192
        ; first byte of the image at 212,0
        ; last byte of first line at 212, 191
        ; last byte of image at 68, 191
        ; Center horizontally
        ldx     #(280-((280-144)/2))
        lda     _div7_table,x
        sta     hgr_start_byte
        lda     _mod7_table,x
        sta     hgr_start_mask

        ; Insert the scaler before the first step (dithering or brightening)
        lda     pixel_handler_first_step+1
        sta     scale_line_done+1
        lda     pixel_handler_first_step+2
        sta     scale_line_done+2
        lda     #<scale_rotated_line
        sta     pixel_handler_first_step+1
        lda     #>scale_rotated_line
        sta     pixel_handler_first_step+2

        lda     #$00
        sta     y_shifter

        ; Patch img_y_to_hgr
        lda     #C_JMP
        sta     img_y_to_hgr
        lda     #<get_scaled_rotated_hgr_byte_from_img_y
        sta     img_y_to_hgr+1
        lda     #>get_scaled_rotated_hgr_byte_from_img_y
        sta     img_y_to_hgr+2

end_90_setup:
        lda     #$01
        sta     hgr_reset_mask

        ldx     #$FF
        stx     x_inverter
        inx
        stx     y_inverter

        ; Patch img_x_to_hgr
        lda     #C_JMP
        sta     img_x_to_hgr
        lda     #<get_rotated_hgr_line_from_img_x
        sta     img_x_to_hgr+1
        lda     #>get_rotated_hgr_line_from_img_x
        sta     img_x_to_hgr+2

        rts

_setup_angle_180:
        ; Set start constants
        lda     #191
        sta     cur_hgr_line
        ; As we don't display thumbnails upside-down,
        ; we are sure to start at column 255
        lda     _centered_div7_table+255
        sta     hgr_start_byte
        lda     _centered_mod7_table+255
        sta     hgr_start_mask
        lda     #$40
        sta     hgr_reset_mask

        ; Patch img_x_to_hgr
        lda     #C_LSR_ZP
        sta     img_x_to_hgr
        lda     #pixel_mask
        sta     img_x_to_hgr+1
        lda     #C_BNE
        sta     img_x_to_hgr+2
        lda     #C_DEY
        sta     hgr_byte_shift

        ; Patch img_y_to_hgr
        lda     #C_LDY_ZP
        sta     img_y_to_hgr
        lda     #cur_hgr_line
        sta     img_y_to_hgr+1
        lda     #C_DEY
        sta     img_y_to_hgr+2

        rts

_setup_angle_270:
        ; Set start constants for counter-clockwise rotation
        lda     #191
        sta     cur_hgr_line

        ldx     _resize
        bne     resize_270_coords
no_resize_270_coords:
        ; The 256x192 will be 192x192, cropped.
        ; first byte of the image at 44, 191
        ; last byte of first line at 44, 0
        ; last byte of image at 236, 0
        ; Center horizontally
        ldx     #(((280-192)/2)+1)
        lda     _div7_table,x
        sta     hgr_start_byte
        lda     _mod7_table,x
        sta     hgr_start_mask

        ; Insert the inverter before the first step (dithering or brightening)
        lda     pixel_handler_first_step+1
        sta     invert_line_done+1
        lda     pixel_handler_first_step+2
        sta     invert_line_done+2
        lda     #<invert_rotated_line
        sta     pixel_handler_first_step+1
        lda     #>invert_rotated_line
        sta     pixel_handler_first_step+2

        ldx     _crop_pos
        lda     crop_shift_270,x
        sta     x_shifter
        lda     crop_low_270,x
        sta     x_invert_low_bound
        lda     crop_high_270,x
        sta     x_invert_high_bound

        lda     #17
        sta     y_shifter

        ; Patch img_y_to_hgr
        lda     #C_JMP
        sta     img_y_to_hgr
        lda     #<get_rotated_hgr_byte_from_img_y
        sta     img_y_to_hgr+1
        lda     #>get_rotated_hgr_byte_from_img_y
        sta     img_y_to_hgr+2

        jmp     end_270_setup
resize_270_coords:
        ; The 256x192 will be 144x192
        ; first byte of the image at 69, 191
        ; last byte of first line at 69, 0
        ; last byte of image at 212, 0
        ; Center horizontally
        ldx     #((280-144)/2)+1
        lda     _div7_table,x
        sta     hgr_start_byte
        lda     _mod7_table,x
        sta     hgr_start_mask

        ldx     #60           ; Should be 68, but *3/4 loses precision
        stx     y_shifter

        ; Insert the scaler before the first step (dithering or brightening)
        lda     pixel_handler_first_step+1
        sta     scale_line_done+1
        lda     pixel_handler_first_step+2
        sta     scale_line_done+2
        lda     #<scale_rotated_line
        sta     pixel_handler_first_step+1
        lda     #>scale_rotated_line
        sta     pixel_handler_first_step+2

        ; Patch img_y_to_hgr
        lda     #C_JMP
        sta     img_y_to_hgr
        lda     #<get_scaled_rotated_hgr_byte_from_img_y
        sta     img_y_to_hgr+1
        lda     #>get_scaled_rotated_hgr_byte_from_img_y
        sta     img_y_to_hgr+2

end_270_setup:
        lda     #$40
        sta     hgr_reset_mask

        ldx     #$00
        stx     x_inverter
        dex
        stx     y_inverter

        ; Patch img_x_to_hgr
        lda     #C_JMP
        sta     img_x_to_hgr
        lda     #<get_rotated_hgr_line_from_img_x
        sta     img_x_to_hgr+1
        lda     #>get_rotated_hgr_line_from_img_x
        sta     img_x_to_hgr+2

        rts

; Check that scaled coords changed, or skip (either a pixel or a whole line)
scale_rotated_line:
        lda     img_x         ; Is it a new line? If not, don't compute scaled Y
        beq     compute_scaled_y
        eor     x_inverter
        MULT_BY_3_DIV_BY_4
        cmp     rotated_x_hgr_line
        beq     skip_scaled_x
        sta     rotated_x_hgr_line
        jmp     scale_line_done

skip_scaled_x:
        dec     img_x
        beq     scale_line_done
        ADVANCE_IMAGE_FAST scale_rotated_line
scale_line_done:
        jmp     $FFFF         ; Patched with next step

compute_scaled_y:
        lda     img_y         ; Compute new scaled Y
        eor     y_inverter
        sec
        sbc     y_shifter
        MULT_BY_3_DIV_BY_4
        cmp     rotated_y_hgr_column
        beq     skip_scaled_line
        sta     rotated_y_hgr_column
        jmp     scale_line_done
skip_scaled_line:
        dec     img_y
        bne     :+
        jmp     all_done
:       jmp     dither_setup_line_start_portrait_skip_prec

; Check that current x is not cropped, or skip (either a pixel or a whole line)
invert_rotated_line:
        lda     img_x         ; Is it a new line? If yes, compute inverted Y
        beq     compute_inverted_y
x_invert_low_bound = *+1
        cmp     #32
        bcc     skip_inverted_x
x_invert_high_bound = *+1
        cmp     #(192+32)
        bcs     skip_inverted_x
        eor     x_inverter
        sec
x_shifter = *+1
        sbc     #32
        sta     rotated_x_hgr_line
        jmp     invert_line_done

skip_inverted_x:
        ldx     img_x
        dex
        beq     invert_line_done
        stx     img_x         ; Store only if non-zero so the main algo can run
        ADVANCE_IMAGE_FAST invert_rotated_line
invert_line_done:
        jmp     $FFFF         ; Patched with next step

compute_inverted_y:
        lda     img_y         ; Compute new scaled Y
        clc
        adc     y_shifter
        eor     y_inverter
        sta     rotated_y_hgr_column
        jmp     invert_line_done


; Get HGR byte and mask for a given image Y
get_scaled_rotated_hgr_byte_from_img_y:
        ldy     rotated_y_hgr_column
        lda     _div7_table+68,y ; Base is at X = 68
        sta     hgr_byte         ; with these computations
        lda     _mod7_table+68,y ; Same
        sta     pixel_mask
        jmp     dither_setup_line_start_portrait

; Get HGR byte and mask for a given image Y
get_rotated_hgr_byte_from_img_y:
        ldy     rotated_y_hgr_column
        lda     _div7_table,y     ; Base is at X = 0
        sta     hgr_byte          ; with these computations
        lda     _mod7_table,y     ; Same
        sta     pixel_mask
        jmp     dither_setup_line_start_portrait

; Get HGR line for a given image Y
get_rotated_hgr_line_from_img_x:
        ; Scale *3/4
        ldx     rotated_x_hgr_line
        ldy     hgr_byte      ; Store what we got
        lda     pixel_val
        sta     (hgr_line),y

        lda     _hgr_baseaddr_l,x
        sta     hgr_line
        lda     _hgr_baseaddr_h,x
        sta     hgr_line+1

        lda     (hgr_line),y  ; Get the existing value
        sta     pixel_val     ; as we re-iterate over the same byte
        jmp     advance_image_byte

patch_dither_branches:
        ; Patch dither branches
        lda     #<(next_line)
        ldx     #>(next_line)
        sta     line_wrapup+1
        stx     line_wrapup+2

        lda     _dither_alg
        beq     set_dither_sierra
        cmp     #DITHER_NONE
        beq     set_dither_none

set_dither_bayer:
        lda     #<(prepare_dither_bayer)
        ldx     #>(prepare_dither_bayer)
        sta     line_prepare_dither_handler+1
        stx     line_prepare_dither_handler+2

        lda     #<(advance_bayer_y)
        ldx     #>(advance_bayer_y)
        sta     line_wrapup+1
        stx     line_wrapup+2

        lda     #<(dither_bayer)
        ldx     #>(dither_bayer)
        jmp     dither_is_set

set_dither_none:
        lda     #<(prepare_dither_none)
        ldx     #>(prepare_dither_none)
        sta     line_prepare_dither_handler+1
        stx     line_prepare_dither_handler+2

        lda     #<(dither_none)
        ldx     #>(dither_none)
        jmp     dither_is_set

set_dither_sierra:
        lda     #<(prepare_dither_sierra)
        ldx     #>(prepare_dither_sierra)
        sta     line_prepare_dither_handler+1
        stx     line_prepare_dither_handler+2

        lda     #<(dither_sierra)
        ldx     #>(dither_sierra)

dither_is_set:
        sta     dither_after_brighten+1 ; Update the jmp at end of brightening
        stx     dither_after_brighten+2 ; to point direct to the ditherer

        ldy     _brighten               ; Do we want to brighten?
        beq     update_pixel_branching
        lda     #<(do_brighten)
        ldx     #>(do_brighten)

update_pixel_branching:
        sta     pixel_handler_first_step+1       ; Update  to either
        stx     pixel_handler_first_step+2       ; the brightener or the ditherer

        rts

        .data

crop_shift_90:   .byte 0,        32,       64
crop_low_90:     .byte 63,       32,       0
crop_high_90:    .byte (192+63), (192+32), 192

crop_shift_270:  .byte 0,        32,       63
crop_low_270:    .byte 0,        32,       63
crop_high_270:  .byte 192,      (192+32), (192+63)

        .bss

pgbar_update:    .res 1
hgr_start_byte:  .res 1
hgr_start_mask:  .res 1
x_center_offset: .res 1

rotated_y_hgr_column: .res 1
rotated_x_hgr_line:   .res 1

x_inverter: .res 1
y_shifter:  .res 1
y_inverter: .res 1
