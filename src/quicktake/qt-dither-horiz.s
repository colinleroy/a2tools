        .export         _setup_angle_0
        .export         _setup_angle_180
        .import         _update_progress_bar
        .export         _do_dither_horiz
        .import         _load_normal_data
        .import         _file_height, _file_width
        .import         _hgr_baseaddr_l, _hgr_baseaddr_h
        .import         clear_dhgr

        .import         _bayer_map, _bayer_map_x, _bayer_map_y
        .import         _end_bayer_map_x, _end_bayer_map_y
        .import         _err_buf
        .import         _opt_histogram
        .import         _is_thumb, _load_thumbnail_data
        .import         _angle
        .import         _brighten, _dither_alg
        .import         _kbhit, _cgetc
        .import         _cur_buf_page, _buffer

        .importzp       img_y, c_sp
        .importzp       _zp2p, _zp4, _zp5, _zp6, _zp7, _zp8, _zp9, _zp10, _zp11, _zp12

        .include "apple2.inc"

        .segment "LC"

thumb_buf_ptr         = _zp2p
img_x                 = _zp4
; img_y               = _zp5, defined in -common.s
pixel_val             = _zp6
pixel_mask            = _zp7
opt_val               = _zp8
err2                  = _zp9
cur_hgr_line          = _zp10

DITHER_NONE       = 2
DITHER_BAYER      = 1
DITHER_SIERRA     = 0
DITHER_THRESHOLD  = $80
DEFAULT_BRIGHTEN  = 0

CENTER_OFFSET       = 0
CENTER_OFFSET_THUMB = 60-12

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
        ; about overflows.
sierra_buf_1:
        lda     safe_err_buf,y
        adc     err2
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
        sta     err2          ; err/2 for (x+1,y)
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
        stx     thumb_buf_ptr+1
        lda     #<(_buffer)
        sta     _cur_buf_page
        sta     thumb_buf_ptr
.endmacro

.macro LOAD_DATA
        sta     $C054
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

        jsr     clear_dhgr

        lda     #16
        sta     pgbar_update

        ; Image lines counter
        lda     _file_height
        sta     img_y

        ldx     #0
        stx     pixel_val

dither_setup_start:
        jsr     patch_dither_branches

        lda     _angle+1
        beq     :+
        jmp     angle270
:       lda     _angle
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

        ; Update line pointer high to new line
        ldx     cur_hgr_line
        lda     _hgr_baseaddr_h,x
hgr_target_line_high:
        sta     hgr_line_ptr_0+2  ; Patched by setup_angle_*

        ; Set line pointer low, accounting for start hgr_byte
        clc
        lda     _hgr_baseaddr_l,x
hgr_start_byte = *+1
        adc      #$00
hgr_target_line_low:
        sta     hgr_line_ptr_0+1  ; Patched by setup_angle_*

        ; Reset start mask
hgr_start_mask = *+1
        lda     #$00
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

buf_ptr_load:
        ldx     _buffer           ; Load, and directly increment the buffer pointer low byte.
        inc     buf_ptr_load+1    ; High byte is taken care of by load_data on every line.

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
store_byte:                           ; It is!
        beq     store_byte_0          ; Branch (patched) to byte storage
                                      ; (beq is condition required to the 180° rotation)
                                      ; but is left as is for 0° rotation as it spares one cycle

store_byte_0:
        lda     #$01                  ; Set init value for mask
        sta     pixel_mask

        tya                           ; Transfer img_x to A for checking which page
        and     #1                    ; to write to and whether to bump hgr_byte
        tax
        lsr                           ; Sets carry for inc or not, and A to 0

        sta     $C054,x

        ldx     pixel_val
hgr_line_ptr_0:
        stx     $FFFF
        sta     pixel_val             ; Reset pixel val

inc_check:
        bcs     pixel_handler         ; Don't shift hgr_byte if wrote AUX (at 0°)
        inc     hgr_line_ptr_0+1
        jmp     pixel_handler

; ===========================================

store_byte_180:
        lda     #$40                  ; Set init value for mask
        sta     pixel_mask

        tya                           ; Transfer img_x to A for checking which page
        and     #1                    ; to write to and whether to bump hgr_byte
        tax
        lsr                           ; Sets carry for dec or not, and A to 0

        sta     $C054,x

        ldx     pixel_val
hgr_line_ptr_180:
        stx     $FFFF
        sta     pixel_val             ; Reset pixel val

dec_check:
        bcc     :+                    ; Don't shift hgr_byte if wrote MAIN (at 180°)
        dec     hgr_line_ptr_180+1
:       jmp     pixel_handler

; ===========================================

increment_high_img_x:
        ldx     img_xh                ; Test if we finished
        bne     line_done             ; Finished the line!
img_x_reinit = *+1
        ldy     #0
        inc     img_xh
        inc     sierra_buf_1+2        ; Bump sierra err buf to second page
        inc     sierra_buf_2+2
        inc     sierra_buf_off_1+2
        inc     sierra_buf_off_2+2
        jmp     img_x_to_hgr          ; Keep going...

; ===========================================

line_done:
        sta     $C054
        PROGRESS_BAR_UPDATE line_wrapup

line_wrapup:
        jmp $FFFF             ; PATCHED with either advance_bayer_y or next_line

advance_bayer_y:
        BAYER_END_LINE

next_line:
        dec     img_y
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
        lda     #$02
        ldx     _is_thumb
        beq     :+
        lda     #$09
:       sta     hgr_start_byte
        lda     #$01
        sta     hgr_start_mask

        ; Patch img_x_to_hgr
        lda     #C_ASL_ZP
        sta     img_x_to_hgr

        ; Patch hgr byte shifter to increase after writing to main
        lda     #C_BPL
        sta     check_store_byte

        .assert (store_byte_0-store_byte-2) < 127, error ; can't branch
        lda     #<(store_byte_0-store_byte-2)
        sta     store_byte+1

        ; Patch img_y_to_hgr
        lda     #C_INC_ZP
        sta     img_y_to_hgr

        ; Patch hgr_line target
        lda     #<(hgr_line_ptr_0+1)
        sta     hgr_target_line_low+1
        lda     #>(hgr_line_ptr_0+1)
        sta     hgr_target_line_low+2

        lda     #<(hgr_line_ptr_0+2)
        sta     hgr_target_line_high+1
        lda     #>(hgr_line_ptr_0+2)
        sta     hgr_target_line_high+2

        rts

_setup_angle_180:
        ; Set start constants
        lda     #191
        sta     cur_hgr_line

        lda     #38
        sta     hgr_start_byte
        lda     #$40
        sta     hgr_start_mask

        ; Patch img_x_to_hgr
        lda     #C_LSR_ZP
        sta     img_x_to_hgr

        ; Patch hgr byte shifter to decrease after writing to aux
        lda     #C_BNE
        sta     check_store_byte

        .assert (store_byte_180-store_byte-2) < 127, error ; can't branch
        lda     #<(store_byte_180-store_byte-2)
        sta     store_byte+1

        ; Patch img_y_to_hgr
        lda     #C_DEC_ZP
        sta     img_y_to_hgr

        ; Patch hgr_line target
        lda     #<(hgr_line_ptr_180+1)
        sta     hgr_target_line_low+1
        lda     #>(hgr_line_ptr_180+1)
        sta     hgr_target_line_low+2

        lda     #<(hgr_line_ptr_180+2)
        sta     hgr_target_line_high+1
        lda     #>(hgr_line_ptr_180+2)
        sta     hgr_target_line_high+2

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


        ; And finally, if brighten == 0 and dither == SIERRA, we can spare one
        ; jump and we'll take it.
        .assert (pixel_handler_first_step-pixel_handler_skip_load_dest-2) < 127, error
        lda     #<(pixel_handler_first_step-pixel_handler_skip_load_dest-2)

        cpy     #0                               ; brighten == 0?
        bne     :+
        ldy     _dither_alg                      ; and dither == Sierra?
        cpy     #DITHER_SIERRA
        bne     :+

        .assert (dither_sierra-pixel_handler_skip_load_dest-2) < 127, error ; can't branch
        lda     #<(dither_sierra-pixel_handler_skip_load_dest-2)

:       sta     pixel_handler_skip_load_dest+1

        rts

pgbar_update:    .res 1
img_xh:          .res 1
