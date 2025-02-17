        .export _draw_palette, _clear_and_draw_palette

        .importzp _zp6p, _zp8, _zp9, _zp10
        .import   mouse_x, mouse_y
        .import   _hgr_hi, _hgr_low
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
        adc     _hgr_low,y
        sta     line
        lda     _hgr_hi,y
        adc     #0
        sta     line+1

        ldy     #((PALETTE_BYTES/PALETTE_HEIGHT)-1)

:       lda     palette_backup,x
        sta     (line),y
        dex
        dey
        bpl     :-

        ldy     cur_y
        iny
        sty     cur_y
        dec     n_lines
        bpl     clear_next_line

        ; Fallback to draw_palette

_draw_palette:
        ; Divide mouse_x by 8 to get the start byte on each line
        lda     mouse_x
        tax
        lsr
        lsr
        lsr
        ; Backup to previous position for next clear
        sta     mouse_prev_x

        ; Select correct sprite
        txa
        and     #$07
        asl
        tax
        lda     _palette,x
        sta     p_pointer+1
        lda     _palette+1,x
        sta     p_pointer+2

        lda     #(PALETTE_HEIGHT-1)
        sta     n_lines

        ldy     mouse_y
        sty     cur_y
        sty     mouse_prev_y

        ldx     #(PALETTE_BYTES-1)

        clc                   ; Clear potential carry from asl

next_line:
        lda     mouse_prev_x  ; Using mouse_prev_x as we just saved it
        adc     _hgr_low,y
        sta     line
        lda     _hgr_hi,y
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

        ldy     cur_y
        iny
        sty     cur_y
        dec     n_lines
        bpl     next_line

        rts

        .bss

palette_backup: .res PALETTE_BYTES
mouse_prev_x:   .res 1
mouse_prev_y:   .res 1
