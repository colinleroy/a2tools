        .export         _setup_angle_0
        .export         _setup_angle_90
        .export         _setup_angle_180
        .export         _setup_angle_270
        .export         _update_progress_bar
        .export         _do_dither
        .export         _load_normal_data
        .import         _load_thumb_data

        .import         _progress_bar, _scrw
        .import         _file_height, _file_width, _off_y, _off_x
        .import         _div7_table, _shifted_div7_table
        .import         _mod7_table, _shifted_mod7_table
        .import         _hgr_baseaddr
        .import         _first_byte_idx, _ptr, _resize, _end_dx, _y, _dy
        .import         _x_offset

        .import         _bayer_map, _bayer_map_x, _bayer_map_y
        .import         _end_bayer_map_x, _end_bayer_map_y
        .import         _err_buf
        .import         _opt_histogram
        .import         _cur_hgr_mod, _cur_hgr_row
        .import         _is_thumb, _load_thumbnail_data
        .import         _prev_scaled_dx, _prev_scaled_dy, _angle
        .import         _brighten, _dither_alg
        .import         _kbhit, _cgetc
        .import         _read_buf, _cur_buf_page, _buffer

        .import         pushax, pusha0, _fread, _ifp, subysp

        .importzp       ptr1, tmp1, tmp2, c_sp, sreg, ptr2
        .importzp       _zp2ip, _zp4p, _zp6s, _zp7, _zp8, _zp9, _zp10, _zp12ip

        .include "apple2.inc"

cur_hgr_baseaddr_ptr  = _zp2ip
thumb_buf_ptr         = _zp4p
pixel                 = _zp6s
opt_val               = _zp7
dx                    = _zp8
err2                  = _zp9
hgr_byte              = _zp10
hgr_line              = _zp12ip

DITHER_NONE       = 2
DITHER_BAYER      = 1
DITHER_SIERRA     = 0
DITHER_THRESHOLD  = $80
DEFAULT_BRIGHTEN  = 0

.macro ADD_UINT16_OFFSET_TO_UINT16_PTR BASE, OFF, RESULT
      lda OFF
      ldx OFF+1
      stx tmp1
      asl a
      rol tmp1
      clc
      adc #<(BASE)
      sta RESULT
      lda tmp1
      adc #>(BASE)
      sta RESULT+1
.endmacro

_setup_angle_0:
        ; off_y = (HGR_HEIGHT - file_height) / 2;
        sec
        lda     #192
        sbc     _file_height
        lsr     a
        sta     _off_y
        lda     #0
        sta     _off_y+1

        ; off_x = 0
        sta     _off_x

        clc
        lda     #<(_div7_table)
        adc     _x_offset
        sta     _shifted_div7_table
        lda     #>(_div7_table)
        adc     #0
        sta     _shifted_div7_table+1

        lda     #<(_mod7_table)
        adc     _x_offset
        sta     _shifted_mod7_table
        lda     #>(_mod7_table)
        adc     #0
        sta     _shifted_mod7_table+1

        ADD_UINT16_OFFSET_TO_UINT16_PTR _hgr_baseaddr, _off_y, cur_hgr_baseaddr_ptr

        lda     _shifted_div7_table
        sta     ptr1
        lda     _shifted_div7_table+1
        sta     ptr1+1

        ldy     #0
        lda     (ptr1),y
        sta     _first_byte_idx
        sta     hgr_byte

        lda     (cur_hgr_baseaddr_ptr),y
        sta     hgr_line
        iny
        lda     (cur_hgr_baseaddr_ptr),y
        sta     hgr_line+1
        rts

_setup_angle_90:
        lda     #0
        sta     _off_x

        lda     _resize
        beq     no_resize90

        lda     #<280
        sta     _off_y
        lda     #>280
        sta     _off_y+1

        lda     _file_width
        sta     _end_dx
        lda     _file_width+1
        sta     _end_dx+1
        rts

no_resize90:
        lda     #<235
        sta     _off_y
        lda     #>235
        sta     _off_y+1

        lda     #192
        sta     _end_dx
        rts

_setup_angle_180:
        ldx     _file_width
        dex
        stx     _off_x
        ldx     _file_height
        dex
        stx     _off_y
        lda #0
        sta     _end_dx
        sta     _off_y+1

        clc
        lda     #<(_div7_table)
        adc     _x_offset
        sta     _shifted_div7_table
        lda     #>(_div7_table)
        adc     #0
        sta     _shifted_div7_table+1

        lda     #<(_mod7_table)
        adc     _x_offset
        sta     _shifted_mod7_table
        lda     #>(_mod7_table)
        adc     #0
        sta     _shifted_mod7_table+1

        ADD_UINT16_OFFSET_TO_UINT16_PTR _hgr_baseaddr, _off_y, cur_hgr_baseaddr_ptr

        clc
        lda     _shifted_div7_table
        adc     _off_x
        sta     ptr1
        lda     _shifted_div7_table+1
        adc     #0
        sta     ptr1+1

        ldy     #0
        lda     (ptr1),y
        sta     _first_byte_idx
        sta     hgr_byte
        
        lda     (cur_hgr_baseaddr_ptr),y
        sta     hgr_line
        iny
        lda     (cur_hgr_baseaddr_ptr),y
        sta     hgr_line+1
        rts

_setup_angle_270:
        lda     _resize
        beq     no_resize270

        ldx     _file_width
        dex
        stx     _off_x

        lda     #<90
        sta     _off_y
        lda     #>90
        sta     _off_y+1

        lda     #0
        sta     _end_dx
        rts

no_resize270:
        ldx     #191
        stx     _off_x
        ldx     #<44
        stx     _off_y
        ldx     #>44
        stx     _off_y+1

        ldx     _file_width
        dex
        stx     _end_dx
        rts

_load_normal_data:
        lda     _y
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
        lda     _y
        sta     (c_sp),y

        lda     #$00
        sta     sreg+1                   ; height (long)
        sta     sreg
        lda     _file_height
        ldx     _file_height+1
        jmp     _progress_bar

_do_dither:
        lda     _file_width
        sta     _end_dx
        lda     #100
        sta     _prev_scaled_dx
        sta     _prev_scaled_dy

dither_setup_start:

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

        ; Patch Sierra error forwarding
        lda     #$88          ; dey
        sta     sierra_err_forward

        ; Patch xcounters direction
        lda     #$E8          ; INX
        sta     dx_incdec
        lda     #$06          ; ASL ZP
        sta     pixel_mask_update
        lda     #$10          ; BPL
        sta     pixel_mask_check
        lda     #$01
        sta     pixel_mask_inival+1
        lda     #$E6          ; INC ZP
        sta     hgr_byte_incdec

        ; Patch ycounters direction
        lda     #<(inc_ycounters)
        ldx     #>(inc_ycounters)
        sta     ycounters_dir+1
        stx     ycounters_dir+2

        ; Patch line coords inversion jmp
        lda     #<(dither_line_start)
        ldx     #>(dither_line_start)
        sta     compute_line_coords+1
        stx     compute_line_coords+2

        ; Patch pixel coords inversion jmp
        lda     #<(dither_pixel_start)
        ldx     #>(dither_pixel_start)
        sta     compute_pixel_coords_a+1
        stx     compute_pixel_coords_a+2

        jmp     finish_patches

angle90:
        jsr     _setup_angle_90

        ; Patch Sierra error forwarding
        lda     #$88          ;dey
        sta     sierra_err_forward

        ; Patch xcounters direction
        lda     #$E8          ; INX
        sta     dx_incdec
        lda     #$06          ; ASL ZP
        sta     pixel_mask_update
        lda     #$10          ; BPL
        sta     pixel_mask_check
        lda     #$01
        sta     pixel_mask_inival+1
        lda     #$E6          ; INC ZP
        sta     hgr_byte_incdec

        ; Patch ycounters direction
        lda     #<(dec_ycounters)
        ldx     #>(dec_ycounters)
        sta     ycounters_dir+1
        stx     ycounters_dir+2

        ; Patch line coords inversion jmp
        lda     #<(invert_line_coords)
        ldx     #>(invert_line_coords)
        sta     compute_line_coords+1
        stx     compute_line_coords+2

        ; Patch pixel coords inversion jmp
        lda     #<(invert_pixel_coords)
        ldx     #>(invert_pixel_coords)
        sta     compute_pixel_coords_a+1
        stx     compute_pixel_coords_a+2
        jmp     finish_patches

angle180:
        jsr     _setup_angle_180
        ; Patch Sierra error forwarding
        lda     #$C8          ; iny
        sta     sierra_err_forward

        ; Patch xcounters direction
        lda     #$CA          ; DEX
        sta     dx_incdec
        lda     #$46          ; LSR ZP
        sta     pixel_mask_update
        lda     #$D0          ; BNE
        sta     pixel_mask_check
        lda     #$40
        sta     pixel_mask_inival+1
        lda     #$C6          ; DEC ZP
        sta     hgr_byte_incdec

        ; Patch ycounters direction
        lda     #<(dec_ycounters)
        ldx     #>(dec_ycounters)
        sta     ycounters_dir+1
        stx     ycounters_dir+2

        ; Patch line coords inversion jmp
        lda     #<(dither_line_start)
        ldx     #>(dither_line_start)
        sta     compute_line_coords+1
        stx     compute_line_coords+2

        ; Patch pixel coords inversion jmp
        lda     #<(dither_pixel_start)
        ldx     #>(dither_pixel_start)
        sta     compute_pixel_coords_a+1
        stx     compute_pixel_coords_a+2
        jmp     finish_patches

angle270:
        jsr     _setup_angle_270

        ; Patch Sierra error forwarding
        lda     #$C8          ; iny
        sta     sierra_err_forward

        ; Patch xcounters direction
        lda     #$CA          ; DEX
        sta     dx_incdec
        lda     #$46          ; LSR ZP
        sta     pixel_mask_update
        lda     #$D0          ; BNE
        sta     pixel_mask_check
        lda     #$40
        sta     pixel_mask_inival+1
        lda     #$C6          ; DEC ZP
        sta     hgr_byte_incdec

        ; Patch ycounters direction
        lda     #<(inc_ycounters)
        ldx     #>(inc_ycounters)
        sta     ycounters_dir+1
        stx     ycounters_dir+2

        ; Patch line coords inversion jmp
        lda     #<(invert_line_coords)
        ldx     #>(invert_line_coords)
        sta     compute_line_coords+1
        stx     compute_line_coords+2

        ; Patch pixel coords inversion jmp
        lda     #<(invert_pixel_coords)
        ldx     #>(invert_pixel_coords)
        sta     compute_pixel_coords_a+1
        stx     compute_pixel_coords_a+2

finish_patches:
        ; Finish patching compute_pixel_coords branches
        lda     compute_pixel_coords_a+1
        ldx     compute_pixel_coords_a+2
        sta     compute_pixel_coords_b+1
        stx     compute_pixel_coords_b+2
        sta     compute_pixel_coords_c+1
        stx     compute_pixel_coords_c+2
        sta     compute_pixel_coords_d+1
        stx     compute_pixel_coords_d+2

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
        sta     prepare_dither+1
        stx     prepare_dither+2

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
        sta     prepare_dither+1
        stx     prepare_dither+2

        lda     #<(dither_none)
        ldx     #>(dither_none)
        jmp     dither_is_set

set_dither_sierra:
        lda     #<(prepare_dither_sierra)
        ldx     #>(prepare_dither_sierra)
        sta     prepare_dither+1
        stx     prepare_dither+2

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
        sta     pixel_branching+1       ; Update the pixel_branching to either
        stx     pixel_branching+2       ; the brightener or the ditherer

        ; Patch shifted mod7 table
        lda     _shifted_mod7_table
        sta     shifted_mod7_load+1
        lda     _shifted_mod7_table+1
        sta     shifted_mod7_load+2

        ; Patch Y loop bound
        lda     _file_height
        sta     y_loop_bound+1

        ; Patch X loop bounds
        lda     _end_dx
        sta     x_loop_bound+1

        ; Line loop setup
        lda     #0
        sta     _bayer_map_y
        sta     _y
        lda     _off_y
        sta     _dy
        lda     _off_y+1
        sta     _dy+1
        lda     #64
        sta     _end_bayer_map_y
        ldx     #>(_buffer)
        stx     _cur_buf_page+1
        stx     thumb_buf_ptr+1
        lda     #<(_buffer)
dither_setup_end:
        sta     _cur_buf_page
        sta     thumb_buf_ptr

dither_setup_line_start:
        ;for (y = 0, dy = off_y; y != file_height;) {
        lda     _is_thumb
        beq     load_normal
        jsr     _load_thumbnail_data
        jmp     prepare_line

load_normal:
        jsr _load_normal_data

prepare_line:
        ldx _cur_buf_page+1
        stx buf_ptr_load+2
        lda _cur_buf_page
        sta buf_ptr_load+1

compute_line_coords:
        jmp     $FFFF         ; PATCHED, will either go to invert_line_coords or dither_line_start
                              ; The cycle benefit is very little, but at least it works the 
                              ; same as for the pixel coords inversion.

invert_line_coords:
        ; Calculate hgr base coordinates for the line
        ldy     _resize
        beq     no_resize

        ;scaled_dy = (dy + (dy << 1)) >> 2;  ; *3/4
        lda     _dy
        ldx     _dy+1
        stx     tmp1
        asl     a
        rol     tmp1

        adc     _dy
        tay                   ; backup low byte
        lda     _dy+1
        adc     tmp1
        lsr     a
        sta     tmp1

        tya
        ror     a
        lsr     tmp1
        ror     a

        cmp     _prev_scaled_dy
        bne     :+
        jmp     next_line
:       sta     _prev_scaled_dy

        tay                   ; scaled_dy not > 255 here
        jmp     set_inv_hgr_ptrs

no_resize:
        ldy     _dy            ; dy not > 255 here
set_inv_hgr_ptrs:
        lda     _div7_table,y ; Not shifted, there*
        sta     _cur_hgr_row
        lda     _mod7_table,y
dither_setup_line_end:
        sta     _cur_hgr_mod

dither_line_start:
        ldy     _off_x
        sty     dx

shifted_mod7_load:
        lda     $FFFF,y       ; Patched at start with *(shifted_mod7_table)
        sta     pixel

prepare_dither:
        jmp     $FFFF         ; PATCHED to prepare_dither_{bayer,sierra,none}

prepare_dither_bayer:
        lda     _bayer_map_y
        sta     _bayer_map_x
        clc
        adc     #8
        sta     _end_bayer_map_x

compute_pixel_coords_a:
        jmp     $FFFF         ; PATCHED, will either go to invert_pixel_coords or dither_pixel_start

prepare_dither_sierra:
        ; reset err2
        lda     #$00
        sta     err2

prepare_dither_none:
        ; Nothing to do here.

        ; Get destination pixel
compute_pixel_coords_b:
        jmp     $FFFF         ; PATCHED, will either go to invert_pixel_coords or dither_pixel_start

invert_pixel_coords:
        ldy     _resize
        beq     no_resize_pixel

        ; scaled_dx = (dx + (dx << 1)) >> 2;  ; *3/4
        lda     #$00
        sta     tmp1
        lda     dx
        asl     a
        rol     tmp1

        adc     dx
        tay                   ; backup low byte

        lda     #0
        adc     tmp1
        lsr     a
        sta     tmp1

        tya
        ror     a
        lsr     tmp1
        ror     a

        cmp     _prev_scaled_dx
        bne     :+
        jmp     next_pixel
:       sta     _prev_scaled_dx

        jmp     inv_pixel_coords

no_resize_pixel:
        lda     dx

inv_pixel_coords:
        ; hgr_line = hgr_baseaddr[scaled_dx] + cur_hgr_row;
        ldx     #>(_hgr_baseaddr)
        asl     a
        tay
        bcc     :+
        inx
        clc
:       lda     #<(_hgr_baseaddr)
        stx     ptr1+1
        sta     ptr1

        lda     (ptr1),y
        adc     _cur_hgr_row
        sta     hgr_line
        iny
        lda     (ptr1),y
        adc     #0
        sta     hgr_line+1

        lda     #0
        sta     hgr_byte

        ;pixel = cur_hgr_mod;
        lda     _cur_hgr_mod
        sta     pixel

dither_pixel_start:
buf_ptr_load:
        ldy     $FFFF         ; Patched with buf_ptr address
        ; opt_val = opt_histogram[*buf_ptr];
        lda     _opt_histogram,y
        sta     opt_val

pixel_branching:
        jmp     $FFFF         ; Will get patched with the most direct thing possible
                              ; (do_brighten, dither_bayer, ...)
dither_bayer:
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
        lda     (hgr_line),y
        ora     pixel
        sta     (hgr_line),y

black_pix_bayer:
        ; Advance Bayer X
        ldx     _bayer_map_x
        inx
        cpx     _end_bayer_map_x
        beq     reset_bayer_x
        stx     _bayer_map_x
        jmp     next_pixel

reset_bayer_x:
        ldx     _bayer_map_y
        stx     _bayer_map_x
        jmp     next_pixel

dither_none:
        lda     opt_val
        cmp     #<DITHER_THRESHOLD
        bcc     next_pixel

        ldy     hgr_byte
        lda     (hgr_line),y
        ora     pixel
        sta     (hgr_line),y

        jmp     next_pixel

dither_sierra:
        ; Add the two errors (the one from previous pixel and the one
        ; from previous line). As they're max 128/2 and 128/4, don't bother
        ; about overflows.
        ldx     #$00
        ldy     dx
        lda     _err_buf,y
        adc     err2
        bpl     err_pos
        dex

err_pos:
      ; Add current pixel value
        clc
        adc     opt_val
        bcc     :+
        inx
        bne     white_pix     ; High byte not zero? - don't check for neg here, can't happen, it's either 0 or 1

:       cpx     #0            ; is high byte negative? (don't check for positive here, can't happen, it's either $FF or 0)
        bmi     forward_err

; Must check low byte
        cmp     #<DITHER_THRESHOLD
        bcc     forward_err_direct
white_pix:
        tax                   ; Backup low byte
        ldy     hgr_byte
        lda     (hgr_line),y
        ora     pixel
        sta     (hgr_line),y
        txa                   ; Restore low byte

forward_err:
        cmp     #$80          ; Keep low byte sign for >> (no need to do it where coming from low byte check path, DITHER_THRESHOLD is $80)
forward_err_direct:
        ror     a
        sta     err2
        cmp     #$80
        ror     a

        ; *(cur_err_x_yplus1+dx)   = err1;
        ldy     dx
        sta     _err_buf,y

        ; if (dx > 0) {
        ;   *(cur_err_x_yplus1+dx-1)    += err1;
        ; }
        beq     next_pixel    ; Is dx == 0

sierra_err_forward:
        dey                   ; Patched with iny at setup, if xdir < 0
        clc                   ; May be set by ror
        adc     _err_buf,y
        sta     _err_buf,y

next_pixel:
        ldx     dx

; The following is heavily patched at startup:
; dec:
;   dx_incdec+1         = DEX
;   pixel_mask_update   = LSR
;   pixel_mask_check    = BNE
;   pixel_mask_inival+1 = $40
;   hgr_byte_incdec     = DEC
; 
; inc:
;   dx_incdec+1         = INC
;   pixel_mask_update   = ASL
;   pixel_mask_check    = BPL
;   pixel_mask_inival+1 = $1
;   hgr_byte_incdec     = INC

dx_incdec:
        dex
x_loop_bound:
        cpx     #$AB          ; PATCHED with end_dx - using AB to avoid optimizer thinking it stays FF
        beq     line_done
        stx     dx

pixel_mask_update:
        lsr     pixel         ; Update pixel mask  (lsr or asl)
pixel_mask_check:
        bne     inc_buf_ptr   ; Check if byte done (bne or bpl)
pixel_mask_inival:
        lda     #$40          ; Set init value     ($40 or $01)
        sta     pixel
hgr_byte_incdec:
        dec     hgr_byte      ; Update hgr byte idx(dec or inc)

; End of mega-patching

inc_buf_ptr:
        inc     buf_ptr_load+1
        beq     inc_buf_ptr_high
compute_pixel_coords_c:
        jmp     $FFFF

inc_buf_ptr_high:
        inc     buf_ptr_load+2
compute_pixel_coords_d:
        jmp     $FFFF

line_done:
        lda     _y
        and     #15

        bne     line_wrapup

        jsr     _update_progress_bar
        jsr     _kbhit
        beq     line_wrapup
        jsr     _cgetc
        cmp     #$1B          ; Esc
        bne     line_wrapup
        rts

line_wrapup:
        jmp $FFFF             ; PATCHED with either advance_bayer_y or next_line

advance_bayer_y:
        ; Advance Bayer Y
        lda     _bayer_map_y
        clc
        adc     #8
        cmp     _end_bayer_map_y
        bne     store_bayer_y

        lda     #0
store_bayer_y:
        sta     _bayer_map_y

next_line:
        lda     cur_hgr_baseaddr_ptr

ycounters_dir:
        jmp     $FFFF         ; PATCHED, to {dec,inc}_ycounters

dec_ycounters:
        sec
        sbc     #2
        sta     cur_hgr_baseaddr_ptr
        bcs     :+
        dec     cur_hgr_baseaddr_ptr+1
:       lda     _dy
        bne     :+
        dec     _dy+1
:       dec     _dy
        jmp     finish_inc

inc_ycounters:
        clc
        adc     #2
        sta     cur_hgr_baseaddr_ptr
        bcc     :+
        inc     cur_hgr_baseaddr_ptr+1
:       inc     _dy
        bne     finish_inc
        inc     _dy+1

finish_inc:
        ldy     #0
        lda     (cur_hgr_baseaddr_ptr),y
        sta     hgr_line
        iny
        lda     (cur_hgr_baseaddr_ptr),y
        sta     hgr_line+1

        lda     _first_byte_idx
        sta     hgr_byte

        inc     _y
        lda     _y
y_loop_bound:
        cmp     #$AB ; PATCHED with file_height
dither_line_end:
        beq     :+
        jmp     dither_setup_line_start
:       rts


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
        clc
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
