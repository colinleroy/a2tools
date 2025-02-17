        .export _draw_palette, _clear_and_draw_palette

        .importzp _zp6p, _zp8, _zp9, _zp10
        .import   mouse_x, mouse_y
        .import   _hgr_baseaddr
        .import   _palette

        .include "apple2.inc"
        .include "palette.inc"

line       = _zp6p

n_lines    = _zp8
sprite_num = _zp9
cur_y      = _zp10

_clear_and_draw_palette:
        lda     #(PALETTE_HEIGHT-1)
        sta     n_lines

        ldy     mouse_prev_y
        sty     cur_y

        ldx     #(PALETTE_BYTES-1)

        clc
clear_next_line:
        lda     mouse_prev_x
        adc     _hgr_baseaddr+256,y
        sta     line
        lda     _hgr_baseaddr+257,y
        adc     #0
        sta     line+1

        ldy     #((PALETTE_BYTES/PALETTE_HEIGHT)-1)

:       lda     palette_backup,x
        sta     (line),y
        dex
        dey
        bpl     :-

        lda     cur_y
        adc     #2
        sta     cur_y
        tay
        dec     n_lines
        bpl     clear_next_line

        ; Fallback to draw_palette

_draw_palette:
        lda     mouse_x
        and     #$07
        asl
        tax
        lda     _palette,x
        sta     p_pointer+1
        lda     _palette+1,x
        sta     p_pointer+2

        lda     #(PALETTE_HEIGHT-1)
        sta     n_lines

        lda     mouse_y
        asl
        sta     cur_y
        sta     mouse_prev_y
        tay                   ; Push to Y for HGR line sel

        ldx     #(PALETTE_BYTES-1)

next_line:
        lda     mouse_x
        lsr
        lsr
        lsr
        sta     mouse_prev_x
        clc                   ; Clear potential carry from lsr
        adc     _hgr_baseaddr+256,y
        sta     line
        lda     _hgr_baseaddr+257,y
        adc     #0
        sta     line+1

        ldy     #((PALETTE_BYTES/PALETTE_HEIGHT)-1)

:       lda     (line),y
        sta     palette_backup,x
        ; draw palette
p_pointer:
        lda     $FFFF,x       ; Patched
        sta     (line),y
        dex
        dey
        bpl     :-

        ; Carry clear here
        lda     cur_y
        adc     #2
        sta     cur_y
        tay
        dec     n_lines
        bpl     next_line

        rts

        .bss

palette_backup: .res PALETTE_BYTES
mouse_prev_x:   .res 1
mouse_prev_y:   .res 1
