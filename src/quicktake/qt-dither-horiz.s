        .export         _setup_angle_0
        .export         _setup_angle_180
        .import         _update_progress_bar
        .export         _do_dither_horiz
        .import         _load_normal_data
        .import         _load_thumb_data

        .import         _file_height, _file_width
        .import         _centered_div7_table
        .import         _centered_mod7_table
        .import         _hgr_baseaddr_l, _hgr_baseaddr_h

        .import         _bayer_map, _bayer_map_x, _bayer_map_y
        .import         _end_bayer_map_x, _end_bayer_map_y
        .import         _err_buf
        .import         _opt_histogram
        .import         _is_thumb, _load_thumbnail_data
        .import         _angle
        .import         _brighten, _dither_alg
        .import         _kbhit, _cgetc
        .import         _read_buf, _cur_buf_page, _buffer

        .import         pushax, pusha0, _fread, _ifp, subysp

        .importzp       img_y, ptr1, tmp1, tmp2, c_sp, sreg, ptr2, ptr4
        .importzp       _zp2p, _zp4, _zp5, _zp6, _zp7, _zp8, _zp9, _zp10, _zp11, _zp12ip

        .include "apple2.inc"

        .segment "LC"

thumb_buf_ptr         = _zp2p
img_x                 = _zp4
pixel_val             = _zp6
pixel_mask            = _zp7
opt_val               = _zp8
err2                  = _zp9
hgr_byte              = _zp10
cur_hgr_line          = _zp11
hgr_line              = _zp12ip
sierra_err_ptr        = ptr4

DITHER_NONE       = 2
DITHER_BAYER      = 1
DITHER_SIERRA     = 0
DITHER_THRESHOLD  = $80
DEFAULT_BRIGHTEN  = 0

CENTER_OFFSET       = 0
CENTER_OFFSET_THUMB = 60-12

sierra_safe_err_buf = _err_buf+2

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

        lda     (sierra_err_ptr),y
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
        sta     (sierra_err_ptr),y

sierra_err_forward:
        dey                   ; Patched with iny at setup, if xdir < 0
        clc                   ; May be set by ror
        adc     (sierra_err_ptr),y
        sta     (sierra_err_ptr),y
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
        lda     img_y
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

_do_dither_horiz:
        bit     $C083         ; WR-enable LC
        bit     $C083

        jsr     clear_dhgr

        lda     #16
        sta     pgbar_update

        ; Image lines counter
        lda     _file_height
        sta     img_y
        ; Image rows counter
        lda     _file_width
        sta     img_x

        lda     #0
        sta     img_xh
        sta     pixel_val

dither_setup_start:
        lda     #CENTER_OFFSET
        ldy     #<dither_setup_line_start_landscape
        ldx     #>dither_setup_line_start_landscape

        ora     _is_thumb
        beq     center_done
        lda     #CENTER_OFFSET_THUMB
        ldy     #<dither_reset_img_x
        ldx     #>dither_reset_img_x

center_done:
        sta     x_center_offset
        sty     next_line_setup+1
        stx     next_line_setup+2

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
        brk

angle180:
        jsr     _setup_angle_180
        jmp     finish_patches

angle270:
        brk

finish_patches:

        ; Line loop setup
        BUFFER_INIT
        BAYER_INIT

dither_reset_img_x:                 ; Needed for thumbnails
        ldy     _file_width
        sty     img_x

dither_setup_line_start_landscape:
        ldy     #0
        sty     img_xh

        ldy     cur_hgr_line
        HGR_SET_LINE_POINTER

        LINE_PREPARE_DITHER
        LOAD_DATA

        lda     #<sierra_safe_err_buf
        sta     sierra_err_ptr
        lda     #>sierra_safe_err_buf
        sta     sierra_err_ptr+1

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
        inc     img_x
        bne     img_x_to_hgr
        ldy     img_xh
        bne     line_done
        iny
        sty     img_xh
        inc     sierra_err_ptr+1

        ; Advance to next HGR pixel after incrementing image X.
        ; The first three bytes are patched according to the rotation of the image.
img_x_to_hgr:
        asl     pixel_mask            ; Update pixel mask  (lsr or asl, or bit if inverted coords)
        bpl     advance_image_byte    ; Check if byte done (bne or bpl)
        HGR_RESET_PIXEL_MASK

store_byte:
        ldy     hgr_byte
        lda     img_x                 ; Check whether we should write MAIN or AUX
        lsr
        bcs     store_main

        sta     CLR80COL
        sta     $C005         ; WRCARDRAM
        HGR_STORE_PIXEL
        sta     $C004         ; WRMAINRAM
        sta     SET80COL
hgr_byte_shift0:
        jmp     advance_image_byte    ; Don't shift hgr_byte if wrote AUX (at 0°)

store_main:
        HGR_STORE_PIXEL
hgr_byte_shift180:
        jmp     hgr_byte_shift

hgr_byte_shift:
        iny
        sty     hgr_byte              ; Update hgr byte idx(dec or inc)

advance_image_byte:
        lda     img_x                 ; Only advance file pointer every two loops
        lsr
        bcs    :+
        jmp    pixel_handler
:       ADVANCE_IMAGE_SLOW pixel_handler

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
        sta     CLR80COL      ; Store the last pixel of the line (assume on AUX)
        sta     $C005         ; WRCARDRAM
        HGR_STORE_PIXEL
        sta     $C004         ; WRMAINRAM
        sta     SET80COL

        ; Advance to the next HGR line after incrementing image Y.
        ; The first three bytes are patched according to the rotation of the image
img_y_to_hgr:
        ldy     cur_hgr_line
        iny
        HGR_SET_LINE_POINTER

next_line_setup:
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
C_BCC    = $90
C_BCS    = $B0

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

clear_hgr_page:
        ldx     #$20
        stx     clear_byte+2
        lda     #$00
        tay
clear_byte:
        sta     $2000,y
        iny
        bne     clear_byte
        inc     clear_byte+2
        dex
        bne     clear_byte
        rts

clear_dhgr:
        jsr     clear_hgr_page

        sta     CLR80COL
        sta     $C005         ; WRCARDRAM
        jsr     clear_hgr_page
        sta     $C004         ; WRMAINRAM
        sta     SET80COL
        rts

_setup_angle_0:
        ; Set start constants
        ; ; Center vertically (for thumbs)
        lda     #192
        sec
        sbc     img_y
        clc
        lsr
        sta     cur_hgr_line
        ; 
        ; Center horizontally
        ; ldx     x_center_offset
        ; lda     _centered_div7_table,x
        lda     #$01
        sta     hgr_start_byte
        ; lda     _centered_mod7_table,x
        lda     #$40
        sta     hgr_start_mask

        lda     #$01
        sta     hgr_reset_mask

        ; Patch hgr byte shifter to increase after writing to main
        lda     #<advance_image_byte
        sta     hgr_byte_shift0+1
        lda     #>advance_image_byte
        sta     hgr_byte_shift0+2
        lda     #<hgr_byte_shift
        sta     hgr_byte_shift180+1
        lda     #>hgr_byte_shift
        sta     hgr_byte_shift180+2

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

_setup_angle_180:
        ; Set start constants
        lda     #191
        sta     cur_hgr_line
        ; As we don't display thumbnails upside-down,
        ; we are sure to start at column 255
        ; lda     _centered_div7_table+255
        lda     #38
        sta     hgr_start_byte
        ; lda     _centered_mod7_table+255
        lda     #$01
        sta     hgr_start_mask
        lda     #$40
        sta     hgr_reset_mask

        ; Patch hgr byte shifter to decrease after writing to aux
        lda     #<hgr_byte_shift
        sta     hgr_byte_shift0+1
        lda     #>hgr_byte_shift
        sta     hgr_byte_shift0+2
        lda     #<advance_image_byte
        sta     hgr_byte_shift180+1
        lda     #>advance_image_byte
        sta     hgr_byte_shift180+2


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

pgbar_update:    .res 1
hgr_start_byte:  .res 1
hgr_start_mask:  .res 1
x_center_offset: .res 1
img_xh:          .res 1
