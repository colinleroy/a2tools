        .export         _setup_angle_90
        .export         _setup_angle_270
        .import         _update_progress_bar
        .export         _do_dither_vert
        .import         _load_normal_data

        .import         _file_height, _file_width
        .import         _hgr_baseaddr_l, _hgr_baseaddr_h
        .import         _resize
        .import         _crop_pos
        .import         _line_buf

        .import         _bayer_map, _bayer_map_x, _bayer_map_y
        .import         _end_bayer_map_x, _end_bayer_map_y
        .import         _err_buf
        .import         _opt_histogram
        .import         _cur_hgr_row
        .import         _is_thumb
        .import         _angle
        .import         _brighten, _dither_alg
        .import         _kbhit, _cgetc
        .import         _read_buf, _cur_buf_page, _buffer

        .import         pushax, pusha0, _fread, _ifp, subysp

        .importzp       img_y, tmp1, tmp2, c_sp
        .importzp       _zp2p, _zp4, _zp5, _zp6, _zp7, _zp8, _zp9, _zp10, _zp11, _zp12ip

        .include "apple2.inc"

img_x                 = _zp4
; img_y               = _zp5, defined in -common.s
pixel_mask            = _zp7
opt_val               = _zp8
err2                  = _zp9
hgr_byte              = _zp10
hgr_line              = _zp12ip

DITHER_NONE       = 2
DITHER_BAYER      = 1
DITHER_SIERRA     = 0
DITHER_THRESHOLD  = $80
DEFAULT_BRIGHTEN  = 0

; defaults:
FIRST_PIXEL_HANDLER = dither_sierra
LINE_DITHER_SETUP   = prepare_dither_sierra

safe_err_buf                = _err_buf+1
; The vertical ditherer only needs one page of Sierra error buffer,
; use the second one to make multiplications faster.
mult_three_quarters_table   = _err_buf+257

; dithering macros.
; They must end with jmp next_pixel, unless
; it's the last one in the loop.
.macro BAYER_DITHER_PIXEL BRANCH_TO
        ldx     _bayer_map_x
        lda     _bayer_map,x
        bpl     positive_b
negative_b:
        adc     opt_val
        bcs     check_bayer_low
        jmp     black_pix_bayer

positive_b:
        adc     opt_val
        bcs     white_pix_bayer
check_bayer_low:
        cmp     #<DITHER_THRESHOLD
        bcc     black_pix_bayer

white_pix_bayer:
        lda     _line_buf,y
        ora     pixel_mask
        sta     _line_buf,y

black_pix_bayer:
        ; Advance Bayer X
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
        lda     _line_buf,y
        ora     pixel_mask
        sta     _line_buf,y
:       jmp     BRANCH_TO
.endmacro

.macro SIERRA_DITHER_PIXEL
        lda     safe_err_buf,y

        adc     err2
        bpl     err_pos
err_neg:
        clc
        adc     opt_val
        bcs     check_low     ; We're not negative anymore
        sec                   ; Still negative. Put sign into carry
        bcs     forward_err   ; (= BRA here)

err_pos:
        ; Add current pixel value
        clc
        adc     opt_val
        bcs     white_pix     ; Overflowed so $FF.

        ; Must check low byte
check_low:
        cmp     #<DITHER_THRESHOLD
        bcc     forward_err
white_pix:
        tax                   ; Backup low byte
        lda     _line_buf,y
        ora     pixel_mask
        sta     _line_buf,y
        txa                   ; Restore low byte

        cmp     #$80          ; Keep low byte sign for >> (no need to do it where coming from low byte check path, DITHER_THRESHOLD is $80)
forward_err:
        ror     a
        sta     err2
        cmp     #$80
        ror     a

        ; *(cur_err_x_yplus1+img_x)   = err1;
        sta     safe_err_buf,y

sierra_err_forward:
        clc                   ; May be set by ror
sierra_revert_1:
        adc     safe_err_buf-1,y
sierra_revert_2:
        sta     safe_err_buf-1,y
.endmacro

; Load a byte from the file. Must end with byte in A.
.macro LOAD_IMAGE_BYTE
buf_ptr_load:
        ldx     $FFFF         ; Patched with buf_ptr address
        ; opt_val = opt_histogram[*buf_ptr];
        lda     _opt_histogram,x
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
        lda     #<(_buffer)
        sta     _cur_buf_page
.endmacro

.macro REPOINT_BUFFER
        ldx     _cur_buf_page+1
        stx     buf_ptr_load+2
        lda     _cur_buf_page
        sta     buf_ptr_load+1
.endmacro

.macro LOAD_DATA
        jsr     _load_normal_data
        REPOINT_BUFFER
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

.macro MULT_BY_3_DIV_BY_4
.scope
        sta     tmp2          ; mult by 1.5
        lsr     a
        clc
        adc     tmp2
        ror     a             ; /2 - is actually good enough
.endscope
.endmacro

; Reset the line buffer
.macro CLEAR_LINE_BUF
.scope
        ldy     #191
        lda     #0
:       sta     _line_buf,y
        dey
        bne     :-
        sta     _line_buf
.endscope
.endmacro

; Commit the current line buffer to the current DHGR row,
; and reset it to zero for the next row.
.macro COMMIT_LINE_BUF
        lda     cur_page          ; Select correct page
page_invert = *+1                 ; Page order differs at 90 and 270°
        eor     #1
        tax
        sta     $C054,x

        ldy     #191
        clc
next_line_slow:                   ; Init (slowly) the line pointer, based
        lda     _hgr_baseaddr_l,y ; on current Y line iteration
        adc     hgr_byte
        sta     line+1
        lda     _hgr_baseaddr_h,y
        sec                       ; For later
next_line_fast:
        sta     line+2            ; Or update it quickly, cf down under

        lda     _line_buf,y       ; Get our byte
line:
        sta     $FFFF             ; Store it
        lda     #0
        sta     _line_buf,y       ; And reset it.

        dey
        beq     :+                ; Are we done?
        lda     line+2
        sbc     #$04              ; Try to decrement line address by $400, and
        cmp     #$20              ; see if we're still in HGR range.
        bcs     next_line_fast
        bcc     next_line_slow    ; Otherwise, reload from Y iterator
:
.endmacro

_do_dither_vert:
        bit     $C083         ; WR-enable LC
        bit     $C083

        CLEAR_LINE_BUF

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
        sta     last_scaled_img_x
        sta     last_scaled_img_y

        ; Create the three-quarters table
        jsr     build_mult_table

dither_setup_start:
        jsr     patch_dither_branches

        ldx     #0
        lda     _resize
        bne     :+
        ldx     _crop_pos
:       lda     crop_start,x
        sta     x_crop_start
        lda     crop_end,x
        sta     x_crop_end
        lda     crop_shift,x
        sta     x_crop_shift


        lda     _angle+1
        bne     angle270

angle90:
        jsr     _setup_angle_90
        jmp     finish_patches

angle270:
        jsr     _setup_angle_270

finish_patches:

        ; Line loop setup
        BUFFER_INIT
        BAYER_INIT

        LOAD_DATA

        lda     #1
        sta     cur_page
        lda     #1
        sta     y_double_loop

line_loop_start:
        ldy     _file_width
        sty     img_x

        ; Compute scaled Y
        lda     img_y
compute_scaled_y:
        bit     y_scale_done
        tax
        lda     mult_three_quarters_table,x
        cmp     last_scaled_img_y
        bne     :+
        ldy     y_double_loop
        beq     :+
        jmp     line_wrapup
:       sta     last_scaled_img_y

y_scale_done:
        ; Shift hgr_mask and/or byte
pixel_mask_update:
        asl     pixel_mask            ; Update pixel mask  (lsr or asl, or bit if inverted coords)
pixel_mask_check:
        bpl     line_prepare_dither   ; Check if byte done (bne or bpl)

hgr_reset_mask = *+1
        lda     #$01                  ; Set init value     ($40 or $01)
        sta     pixel_mask

        COMMIT_LINE_BUF               ; New seven pixels row finished, commit buffer

        dec     cur_page              ; Should we shift the HGR byte or just change page?
        bpl     line_prepare_dither

hgr_byte_shift:
        inc     hgr_byte
        lda     #1
        sta     cur_page

line_prepare_dither:
        LINE_PREPARE_DITHER

pixel_loop_start:
pixel_handler:
        LOAD_IMAGE_BYTE

next_pixel:
        lda     img_x
x_crop_start = *+1
        cmp     #$00
        bcs     compute_scaled_x
        jmp     next_x

        ; Compute scaled X
compute_scaled_x:
        bit     x_scale_done
        tax
        lda     mult_three_quarters_table,x
        cmp     last_scaled_img_x
        bne     x_scale_done
        jmp     next_x

x_scale_done:
        sta     last_scaled_img_x
x_crop_end = *+1
        cmp     #191
        bne     shift_x
        jmp     line_done

shift_x:
        sec
x_crop_shift = *+1
        sbc     #$00

mirror_x:
        bit     do_mirror_x
mirror_x_done:
        tay                         ; final_img_x now in Y, keep it there for the whole loop


pixel_handler_first_step:
        jmp     FIRST_PIXEL_HANDLER ; Will get patched with the most
                                    ; direct thing possible
                                    ; (do_brighten, dither_bayer, ...)

; Sierra inlined to spare a jump. No dither and bayer are out of the
; main code path.
dither_sierra:
        SIERRA_DITHER_PIXEL

next_x:
        inc     img_x
        ADVANCE_IMAGE_SLOW pixel_handler

line_done:
        sta     $C054
        PROGRESS_BAR_UPDATE line_wrapup

line_wrapup:
        jmp $FFFF             ; PATCHED with either advance_bayer_y or next_line

advance_bayer_y:
        BAYER_END_LINE

next_line:
        dec     y_double_loop
        bpl     redo_line

        lda     #1
        sta     y_double_loop

        dec     img_y
        beq     all_done

        LOAD_DATA
        jmp     line_loop_start

all_done:
        rts

redo_line:
        REPOINT_BUFFER
        jmp     line_loop_start

do_mirror_x:
        eor     #$FF
        sec
        sbc     #64
        jmp     mirror_x_done

dither_none:
        NO_DITHER_PIXEL     next_x
dither_bayer:
        BAYER_DITHER_PIXEL  next_x

; Brightening, out of the main code path
do_brighten:
        lda     _brighten
        bpl     brighten_pos
brighten_neg:
        adc     opt_val
        bcs     store_opt
        lda     #0
        jmp     store_opt
brighten_pos:
        adc     opt_val
        bcc     store_opt
        lda     #$FF

store_opt:
        sta     opt_val
dither_after_brighten:
        jmp     $FFFF

; ; Rotation setup. Will patch
; ; - img_x_to_hgr
; ; - img_y_to_hgr
; ; - incs, decs, etc
;
C_ASL_ZP  = $06
C_LSR_ZP  = $46
C_BPL     = $10
C_BEQ     = $F0
C_BNE     = $D0
C_INC_ZP  = $E6
C_DEC_ZP  = $C6
C_LDY_ZP  = $A4
C_INY     = $C8
C_DEY     = $88
C_JMP     = $4C
C_BIT_ABS = $2C
;

_setup_angle_90:
        ; Set start constants for clockwise rotation
        lda     #1
        sta     page_invert

        lda     #C_DEC_ZP
        sta     hgr_byte_shift
        lda     #$40
        sta     hgr_reset_mask

        lda     #C_LSR_ZP
        sta     pixel_mask_update

        lda     #C_BNE
        sta     pixel_mask_check

        lda     #C_BIT_ABS
        sta     mirror_x

        lda     #<(safe_err_buf-1)
        sta     sierra_revert_1+1
        sta     sierra_revert_2+1

        ldx     _resize
        bne     resize_90_coords
no_resize_90_coords:
        ; The 256x192 will be 192x192, cropped.
        ; first byte of the image at 236,0
        ; last byte of first line at 236, 191
        ; last byte of image at 32, 191
        lda     #32
        sta     hgr_byte

        lda     #$40
        sta     pixel_mask

        lda     #C_JMP
        sta     compute_scaled_y
        sta     compute_scaled_x
        rts

resize_90_coords:
        ; The 256x192 will be 144x192
        ; first byte of the image at 212,0
        ; last byte of first line at 212, 191
        ; last byte of image at 68, 191
        lda     #30
        sta     hgr_byte

        lda     #$40
        sta     pixel_mask

        lda     #C_BIT_ABS
        sta     compute_scaled_y
        sta     compute_scaled_x

        rts

_setup_angle_270:
        ; Set start constants for counter-clockwise rotation
        lda     #0
        sta     page_invert

        lda     #C_INC_ZP
        sta     hgr_byte_shift

        lda     #01
        sta     hgr_reset_mask

        lda     #C_ASL_ZP
        sta     pixel_mask_update

        lda     #C_BPL
        sta     pixel_mask_check

        lda     #C_JMP
        sta     mirror_x

        lda     #<(safe_err_buf+1)  ; We iterate lines the other way so
        sta     sierra_revert_1+1   ; shift the x-1 buffer.
        sta     sierra_revert_2+1

        ldx     _resize
        bne     resize_270_coords
no_resize_270_coords:
        ; ; The 256x192 will be 192x192, cropped.
        ; ; first byte of the image at 44, 191
        ; ; last byte of first line at 44, 0
        ; ; last byte of image at 236, 0
        lda     #6
        sta     hgr_byte

        lda     #$01
        sta     pixel_mask

        lda     #C_JMP
        sta     compute_scaled_y
        sta     compute_scaled_x

        rts

resize_270_coords:
        ; The 256x192 will be 144x192
        ; first byte of the image at 69, 191
        ; last byte of first line at 69, 0
        ; last byte of image at 212, 0
        lda     #7
        sta     hgr_byte

        lda     #$01
        sta     pixel_mask

        lda     #C_BIT_ABS
        sta     compute_scaled_y
        sta     compute_scaled_x

        rts

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
        sta     pixel_handler_first_step+1       ; Update to either
        stx     pixel_handler_first_step+2       ; the brightener or the ditherer

        rts

build_mult_table:
        ldy      #0
next:
        tya
        MULT_BY_3_DIV_BY_4
        sta      mult_three_quarters_table,y
        iny
        bne      next
        rts

crop_start:      .byte 0,        32,       64
crop_end:        .byte 191,      191+32,   191+64
crop_shift:      .byte 0,        32,       64

pgbar_update:    .res 1

last_scaled_img_x:  .res 1
last_scaled_img_y:  .res 1

cur_page:           .res 1
y_double_loop:      .res 1
