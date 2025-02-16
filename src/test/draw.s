        .export _draw

        .importzp _zp6p, _zp8p, _zp10, _zp11, _zp12
        .import   mouse_x, mouse_y
        .import   _hgr_baseaddr
        .import   _palette

        .include "apple2.inc"

sprite     = _zp6p
line       = _zp8p

n_lines    = _zp10
sprite_num = _zp11
cur_y      = _zp12

_draw:
        lda     mouse_x
        and     #$07
        asl
        tax
        lda     _palette,x
        sta     sprite
        lda     _palette+1,x
        sta     sprite+1

        lda     #14
        sta     n_lines

        lda     mouse_y
        asl
        sta     cur_y

next_line:
        lda     mouse_x
        lsr
        lsr
        lsr
        clc
        ldx     cur_y
        adc     _hgr_baseaddr+256,x
        sta     line
        lda     _hgr_baseaddr+257,x
        adc     #0
        sta     line+1

        ldy     #5

:       lda     (sprite),y
        sta     (line),y
        dey
        bpl     :-

        clc
        lda     sprite
        adc     #6
        sta     sprite
        lda     sprite+1
        adc     #0
        sta     sprite+1

        inc     cur_y
        inc     cur_y
        dec     n_lines
        bpl     next_line

        rts
