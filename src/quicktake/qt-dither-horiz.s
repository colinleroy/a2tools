        .export         _setup_angle_0
        .export         _setup_angle_180
        .import         _update_progress_bar
        .export         _do_dither_horiz
        .import         _load_normal_data
        .import         _file_height, _file_width
        .import         _hgr_baseaddr_l, _hgr_baseaddr_h

        .import         _bayer_map, _bayer_map_x, _bayer_map_y
        .import         _end_bayer_map_x, _end_bayer_map_y
        .import         _err_buf
        .import         _opt_histogram
        .import         _is_thumb, _load_thumbnail_data
        .import         _angle
        .import         _brighten, _dither_alg
        .import         _kbhit, _cgetc
        .import         _cur_buf_page, _buffer, _line_buf

        .importzp       img_y, c_sp, ptr3
        .importzp       _zp2p, _zp4, _zp5, _zp6, _zp7, _zp8, _zp9, _zp10, _zp11, _zp12

        .include "apple2.inc"

        .segment "LC"

img_x                 = _zp4
; img_y               = _zp5, defined in -common.s
pixel_val             = _zp6
pixel_mask            = _zp7
opt_val               = _zp8
err_nextx             = _zp9
cur_hgr_line          = _zp10

DITHER_NONE       = 2
DITHER_BAYER      = 1
DITHER_SIERRA     = 0
DITHER_THRESHOLD  = $80
DEFAULT_BRIGHTEN  = 0

DHGR_LINE_LEN     = 72
HGR_LINE_LEN      = 36

THUMBNAIL_BUFFER_OFFSET = ((256-160)/2)

; defaults:
FIRST_PIXEL_HANDLER = dither_sierra
LINE_DITHER_SETUP   = prepare_dither_sierra

safe_err_buf = _err_buf+1

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
        lda     pixel_val
        ora     pixel_mask
        sta     pixel_val

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
        lda     pixel_val
        ora     pixel_mask
        sta     pixel_val
:       jmp     BRANCH_TO
.endmacro

.macro SIERRA_DITHER_PIXEL
        ; Add the two errors (the one from previous pixel and the one
        ; from previous line). As they're max 128/2 and 128/4, don't bother
        ; about overflows at that point, but care about the sign.
sierra_buf_1:
        lda     safe_err_buf,y
        adc     err_nextx
        bpl     err_pos
err_neg:
        clc
        adc     opt_val
        bcs     check_low     ; We're not negative anymore
        sec                   ; Still negative. Put sign into carry
        jmp     forward_err   ; the pixel is black, just forward the error to next line

err_pos:
        clc
        adc     opt_val
        bcs     white_pix     ; Overflowed so the pixel is white.

check_low:                    ; No overflow, must check low byte for threshold
        cmp     #<DITHER_THRESHOLD
        bcc     forward_err   ; Pixel is black!

white_pix:
        tax                   ; Backup our pixel + err(x+1,y)+err(x,y+1) calculation
        lda     pixel_val
        ora     pixel_mask
        sta     pixel_val
        txa                   ; Restore it

        cmp     #$80          ; Set carry according to sign (needed when coming from err_pos)

forward_err:                  ; And forward error to next pixels
        ror     a
        sta     err_nextx     ; err/2 for (x+1,y)
        cmp     #$80
        ror     a

sierra_buf_2:
        ; err/4 for x,y+1
        sta     safe_err_buf,y

        ; previous err + err/4 for x-1,y+1
        clc                   ; May be set by ror
sierra_buf_off_1:
        adc     safe_err_buf-1,y
sierra_buf_off_2:
        sta     safe_err_buf-1,y
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
        jmp     prepare_dither_none

prepare_dither_sierra:
        ; Reset err buf to start
        lda     #>(safe_err_buf)
        sta     sierra_buf_1+2
        sta     sierra_buf_2+2
        lda     #>(safe_err_buf-1)
        sta     sierra_buf_off_1+2
        sta     sierra_buf_off_2+2
        ; reset err_nextx
        lda     #$00
        sta     err_nextx

prepare_dither_none:          ; Nothing to do here.
.endmacro

.macro BAYER_INIT
        lda     #0
        sta     _bayer_map_y
        lda     #64
        sta     _end_bayer_map_y
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


.assert <_buffer = 0, error ; Buffer must be aligned

.macro BUFFER_INIT
        ldx     #>(_buffer)
        stx     _cur_buf_page+1
        lda     #<(_buffer)
        sta     _cur_buf_page
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
        lda     #<(_buffer)
        sta     buf_ptr_load+1
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
        rts                   ; Exit _do_dither_horiz
.endmacro

_do_dither_horiz:
        bit     $C083         ; WR-enable LC
        bit     $C083

        lda     #16
        sta     pgbar_update

        ; Image lines counter
        lda     _file_height
        sta     img_y

        ldx     #0
        stx     pixel_val

dither_setup_start:
        jmp     patch_dither_branches

dither_branches_patched:
        lda     _angle
        beq     angle0
        jmp     _setup_angle_180
angle0:
        jmp     _setup_angle_0

finish_patches:

        ; Initial setup
        BUFFER_INIT
        BAYER_INIT

        lda     #0
        ldx     _is_thumb
        beq     :+
        sec
        sbc     _file_width       ; Patch X bound check for thumbs

:       sta     img_x_init
        sta     img_x_reinit

; Line loop start
dither_setup_line_start_landscape:
        ldx     #0
        stx     img_xh

        ; Set initial position in line buffer
line_buf_start_byte = *+1
        lda     #$00
        sta     line_buf_ptr+1

        ; Reset start mask
        lda     hgr_start_mask
        sta     pixel_mask

        LINE_PREPARE_DITHER
        LOAD_DATA

img_x_init = *+1
        ldy     #0                ; We'll keep Y = img_x in the whole row loop.

pixel_handler:
        tya                       ; Increment buffer only every two DHGR pixels
        lsr
pixel_handler_skip_load_dest:     ; Maybe directly patched with bcs dither_sierra if _brigten == 0
        bcs     pixel_handler_first_step

        tax
buf_ptr_load:
        lda     _buffer,x         ; Load byte from buffer.
        tax
        lda     _opt_histogram,x  ; Get histogram-optimized value
        sta     opt_val

pixel_handler_first_step:
        jmp     FIRST_PIXEL_HANDLER ; Will get patched with the most
                                    ; direct thing possible
                                    ; (do_brighten, dither_bayer, ...)

; Sierra inlined to spare a jump. No dither and bayer are out of the
; main code path.
dither_sierra:
        SIERRA_DITHER_PIXEL

next_pixel:
        iny
        beq     increment_high_img_x  ; This will exit to line_done after 512 pixels (256*2) or 160 (160*1)

        ; Advance to next HGR pixel after incrementing image X.
        ; The opcodes and targets are patched according to the rotation of the image.
img_x_to_hgr:
        asl     pixel_mask            ; Update pixel mask  (lsr or asl, or bit if inverted coords)
check_store_byte:
        bpl     pixel_handler         ; Check if byte done (bpl or bne, patched)

        ; Byte done, store it in the line buffer
hgr_start_mask = *+1
        lda     #$01                  ; Set init value for mask
        sta     pixel_mask

        lda     pixel_val
line_buf_ptr:
        sta     _line_buf             ; Store our value
        lda     #$0
        sta     pixel_val             ; Reset pixel val

shift_line_buf:
        inc     line_buf_ptr+1
        jmp     pixel_handler

increment_high_img_x:
        ldx     img_xh                ; Test if we finished
        bne     line_done             ; Finished the line!
img_x_reinit = *+1
        ldy     #0
        inc     img_xh
read_buffer_bump = *+1
        lda     #<(_buffer+$80)       ; Bump the buffer loader to second half of page
        sta     buf_ptr_load+1
        inc     sierra_buf_1+2        ; Bump sierra err buf to second page
        inc     sierra_buf_2+2
        inc     sierra_buf_off_1+2
        inc     sierra_buf_off_2+2
        jmp     img_x_to_hgr          ; Keep going...

; ===========================================

line_done:
        PROGRESS_BAR_UPDATE line_wrapup

line_wrapup:
        jmp $FFFF             ; PATCHED with either advance_bayer_y or next_line

advance_bayer_y:
        BAYER_END_LINE

next_line:
        ; Copy line buf to DHGR screen
        ldx     cur_hgr_line
        lda     _hgr_baseaddr_l,x
        sta     hgr_line_ptr_a+1
        sta     hgr_line_ptr_b+1
        lda     _hgr_baseaddr_h,x
        sta     hgr_line_ptr_a+2
        sta     hgr_line_ptr_b+2

        ; Odd bytes to MAIN
        ldy     #(HGR_LINE_LEN+1)
        ldx     #DHGR_LINE_LEN
:       lda     _line_buf,x
hgr_line_ptr_a:
        sta     $FFFF,y
        dey
        dex
        dex
        bpl     :-

        ; Even bytes to AUX
        sta     $C055
        ldy     #(HGR_LINE_LEN+1)
        ldx     #(DHGR_LINE_LEN-1)
:       lda     _line_buf,x
hgr_line_ptr_b:
        sta     $FFFF,y
        dey
        dex
        dex
        bpl     :-
        sta     $C054

        dec     img_y             ; Are we done??
        beq     all_done

        ; Advance to the next HGR line after incrementing image Y.
        ; The inc(/dec) is patched according to the rotation of the image
img_y_to_hgr:
        inc     cur_hgr_line
        jmp     dither_setup_line_start_landscape

all_done:
        rts

dither_none:
        NO_DITHER_PIXEL     next_pixel
dither_bayer:
        BAYER_DITHER_PIXEL  next_pixel

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

; Rotation setup. Will patch
; - img_x_to_hgr
; - img_y_to_hgr
; - incs, decs, etc

C_ASL_ZP = $06
C_LSR_ZP = $46
C_BPL    = $10
C_BMI    = $30
C_BNE    = $D0
C_BEQ    = $F0
C_INC_ZP = $E6
C_DEC_ZP = $C6
C_LDX_ZP = $A6
C_LDY_ZP = $A4
C_DEC_ABS= $CE
C_INC_ABS= $EE
C_INY    = $C8
C_DEY    = $88
C_INX    = $E8
C_DEX    = $CA
C_JMP    = $4C
C_BCC    = $90
C_BCS    = $B0

_setup_angle_0:
        ; Set start constants
        ; ; Center vertically (for thumbs)
        lda     #192
        sec
        sbc     img_y
        lsr
        sta     cur_hgr_line

        ; Center horizontally
        lda     #<_line_buf       ; Full size image: Start a first byte of line buffer
        ldy     #<(_buffer+$80)   ; How much to bump the buffer to read mid-line: Half-width of full-size image
        ldx     _is_thumb
        beq     :+
        lda     #<(_line_buf+16)  ; Thumbnail: start at byte 16 to center on-screen
        ldy     #<(_buffer+$80-THUMBNAIL_BUFFER_OFFSET) ; Thumbnails: bump the buffer taking X-loop-optimizing offset into account
:
        sta     line_buf_start_byte
        sty     read_buffer_bump

        lda     #$01
        sta     hgr_start_mask

        ; Patch img_x_to_hgr
        lda     #C_ASL_ZP
        sta     img_x_to_hgr

        ; Patch hgr byte shifter to increase after writing to main
        lda     #C_BPL
        sta     check_store_byte

        ; Patch img_y_to_hgr
        lda     #C_INC_ZP
        sta     img_y_to_hgr

        lda     #C_INC_ABS
        sta     shift_line_buf

        jmp     finish_patches

_setup_angle_180:
        ; Set start constants
        lda     #191
        sta     cur_hgr_line

        lda     #<(_line_buf+DHGR_LINE_LEN)
        sta     line_buf_start_byte
        lda     #$40
        sta     hgr_start_mask

        ; Patch img_x_to_hgr
        lda     #C_LSR_ZP
        sta     img_x_to_hgr

        ; Patch hgr byte shifter to decrease after writing to aux
        lda     #C_BNE
        sta     check_store_byte

        ; Patch img_y_to_hgr
        lda     #C_DEC_ZP
        sta     img_y_to_hgr

        lda     #C_DEC_ABS
        sta     shift_line_buf

        jmp     finish_patches

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


        ; And finally, if brighten == 0 and dither == SIERRA, we can spare one
        ; jump and we'll take it.

        .assert (pixel_handler_first_step-pixel_handler_skip_load_dest-2) < 127, error
        ; Load default target for when skipping byte load
        lda     #<(pixel_handler_first_step-pixel_handler_skip_load_dest-2)

        cpy     #0                               ; brighten == 0?
        bne     :+
        ldy     _dither_alg                      ; and dither == Sierra?
        cpy     #DITHER_SIERRA
        bne     :+

        .assert (dither_sierra-pixel_handler_skip_load_dest-2) < 127, error ; can't branch
        ; Load direct target to sierra ditherer
        lda     #<(dither_sierra-pixel_handler_skip_load_dest-2)

:       sta     pixel_handler_skip_load_dest+1
        jmp     dither_branches_patched

pgbar_update:    .res 1
img_xh:          .res 1
