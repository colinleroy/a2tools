        .export _draw_plane, _clear_and_draw_plane

        .importzp _zp6p, _zp8, _zp9, _zp10
        .import   mouse_x, mouse_y
        .import   _hgr_hi, _hgr_low
        .import   _plane, _plane_mask

        .include "apple2.inc"
        .include "plane.inc"

line       = _zp6p

n_lines    = _zp8
sprite_num = _zp9
cur_y      = _zp10

_clear_and_draw_plane:
        lda     #(plane_HEIGHT-1)
        sta     n_lines

        ldy     mouse_prev_y
        sty     cur_y

        ldx     #(plane_BYTES-1)

        clc
clear_next_line:
        lda     mouse_prev_x
        adc     _hgr_low,y
        sta     line
        lda     _hgr_hi,y
        adc     #0
        sta     line+1

        ldy     #((plane_BYTES/plane_HEIGHT)-1)

:       lda     plane_backup,x
        sta     (line),y
        dex
        dey
        bpl     :-

        inc     cur_y
        ldy     cur_y
        dec     n_lines
        bpl     clear_next_line

        ; Fallback to draw_plane

_draw_plane:
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
        lda     _plane,x
        sta     p_pointer+1
        lda     _plane+1,x
        sta     p_pointer+2

        lda     _plane_mask,x
        sta     p_mask+1
        lda     _plane_mask+1,x
        sta     p_mask+2

        lda     #(plane_HEIGHT-1)
        sta     n_lines

        ldy     mouse_y
        sty     cur_y
        sty     mouse_prev_y

        ldx     #(plane_BYTES-1)

        clc                   ; Clear potential carry from asl

next_line:
        lda     mouse_prev_x  ; Using mouse_prev_x as we just saved it
        adc     _hgr_low,y
        sta     line
        lda     _hgr_hi,y
        adc     #0
        sta     line+1

        ldy     #((plane_BYTES/plane_HEIGHT)-1)

:       lda     (line),y
        sta     plane_backup,x
        ; draw plane
p_mask:
        and     $FFFF,x       ; Patched
p_pointer:
        ora     $FFFF,x       ; Patched
        sta     (line),y
        dex
        dey
        bpl     :-

        inc     cur_y
        ldy     cur_y
        dec     n_lines
        bpl     next_line

        rts

        .bss

plane_backup: .res plane_BYTES
mouse_prev_x:   .res 1
mouse_prev_y:   .res 1
