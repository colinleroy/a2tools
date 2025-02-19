        .export _draw_plane, _clear_and_draw_plane

        .importzp _zp6p, _zp8, _zp9, _zp10
        .import   _hgr_hi, _hgr_low
        .import   _plane, _plane_mask
        .import   _div7_table, _mod7_table

        .include "apple2.inc"
        .include "plane.inc"

line       = _zp6p
n_lines    = _zp8
sprite_num = _zp9
cur_y      = _zp10

; X, Y : coordinates
_clear_and_draw_plane:
        stx     plane_x       ; Backup coordinates
        sty     plane_y

        lda     #(plane_HEIGHT-1)
        sta     n_lines

        ldy     prev_plane_y
        sty     cur_y

        ldx     #(plane_BYTES-1)

        clc
clear_next_line:
        lda     prev_plane_x
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
        ldx     plane_x
        ldy     plane_y

; X, Y : coordinates
_draw_plane:
        sty     cur_y
        sty     prev_plane_y
        ; Divide X by 8 to get the start byte on each line
        txa

        lda     _div7_table,x
        ; Backup to previous position for next clear
        sta     prev_plane_x

        ; Select correct sprite
        lda     _mod7_table,x
        asl
        tax
        lda     _plane,x
        sta     plane_pointer+1
        lda     _plane+1,x
        sta     plane_pointer+2

        lda     _plane_mask,x
        sta     plane_mask+1
        lda     _plane_mask+1,x
        sta     plane_mask+2

        lda     #(plane_HEIGHT-1)
        sta     n_lines

        ldx     #(plane_BYTES-1)

        clc                   ; Clear potential carry from asl
next_line:
        lda     prev_plane_x  ; Using prev_plane_x as we just saved it
        adc     _hgr_low,y
        sta     line
        lda     _hgr_hi,y
        adc     #0
        sta     line+1

        ldy     #((plane_BYTES/plane_HEIGHT)-1)

:       lda     (line),y
        sta     plane_backup,x
        ; draw plane
plane_mask:
        and     $FFFF,x       ; Patched
plane_pointer:
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

plane_backup:        .res plane_BYTES
plane_x:             .res 1
plane_y:             .res 1
prev_plane_x:        .res 1
prev_plane_y:        .res 1
