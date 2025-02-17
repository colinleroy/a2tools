        .export _draw_palette, _clear_palette

        .importzp _zp6p, _zp8p, _zp10, _zp11, _zp12, ptr1
        .import   mouse_x, mouse_y
        .import   _hgr_baseaddr
        .import   _palette

        .include "apple2.inc"
        .include "palette.inc"

sprite     = _zp6p
line       = _zp8p

n_lines    = _zp10
sprite_num = _zp11
cur_y      = _zp12

_clear_palette:
        lda     #14
        sta     n_lines

        lda     mouse_prev_y
        sta     cur_y

        ldx     #(PALETTE_BYTES-1)

clear_next_line:
        lda     mouse_prev_x
        clc
        ldy     cur_y
        adc     _hgr_baseaddr+256,y
        sta     line
        lda     _hgr_baseaddr+257,y
        adc     #0
        sta     line+1

        ldy     #5

:       lda     palette_backup,x
        sta     (line),y
        dex
        dey
        bpl     :-

        clc
        lda     cur_y
        adc     #2
        sta     cur_y
        dec     n_lines
        bpl     clear_next_line

        rts


_draw_palette:
        lda     mouse_x
        and     #$07
        asl
        tax
        lda     _palette,x
        sta     p_pointer+1
        lda     _palette+1,x
        sta     p_pointer+2

        lda     #14
        sta     n_lines

        lda     mouse_y
        asl
        sta     cur_y
        sta     mouse_prev_y

        ldx     #(PALETTE_BYTES-1)

next_line:
        lda     mouse_x
        lsr
        lsr
        lsr
        sta     mouse_prev_x
        clc
        ldy     cur_y
        adc     _hgr_baseaddr+256,y
        sta     line
        lda     _hgr_baseaddr+257,y
        adc     #0
        sta     line+1

        ldy     #5

:       lda     (line),y
        sta     palette_backup,x
        ; draw palette
p_pointer:
        lda     $FFFF,x
        sta     (line),y
        dex
        dey
        bpl     :-

        clc
        lda     cur_y
        adc     #2
        sta     cur_y

        dec     n_lines
        bpl     next_line

        rts

        .bss

palette_backup: .res PALETTE_BYTES
mouse_prev_x:   .res 1
mouse_prev_y:   .res 1
