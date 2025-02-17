        .export _draw

        .importzp _zp6p, _zp8p, _zp10, _zp11, _zp12, ptr1
        .import   mouse_x, mouse_y
        .import   _hgr_baseaddr
        .import   _palette

        .include "apple2.inc"

sprite     = _zp6p
line       = _zp8p

n_lines    = _zp10
sprite_num = _zp11
cur_y      = _zp12

backup_ptr = ptr1

_clear:
        lda     #<palette_backup
        sta     backup_ptr
        lda     #>palette_backup
        sta     backup_ptr+1

        lda     #14
        sta     n_lines

        lda     mouse_prev_y
        sta     cur_y

clear_next_line:
        lda     mouse_prev_x
        clc
        ldx     cur_y
        adc     _hgr_baseaddr+256,x
        sta     line
        lda     _hgr_baseaddr+257,x
        adc     #0
        sta     line+1

        ldy     #5

:       lda     (backup_ptr),y
        sta     (line),y
        dey
        bpl     :-

        clc
        ; Increment backup line
        lda     backup_ptr
        adc     #6
        sta     backup_ptr
        lda     backup_ptr+1
        adc     #0
        sta     backup_ptr+1

        lda     cur_y
        adc     #2
        sta     cur_y
        dec     n_lines
        bpl     clear_next_line

        rts


_draw:
        lda     mouse_x
        and     #$07
        asl
        tax
        lda     _palette,x
        sta     sprite
        lda     _palette+1,x
        sta     sprite+1

        ; Clear previous sprite
        jsr     _clear

        lda     #14
        sta     n_lines

        lda     mouse_y
        asl
        sta     cur_y
        sta     mouse_prev_y

        ; Init backup_ptr
        lda     #<palette_backup
        sta     backup_ptr
        lda     #>palette_backup
        sta     backup_ptr+1

next_line:
        lda     mouse_x
        lsr
        lsr
        lsr
        sta     mouse_prev_x
        clc
        ldx     cur_y
        adc     _hgr_baseaddr+256,x
        sta     line
        lda     _hgr_baseaddr+257,x
        adc     #0
        sta     line+1

        ldy     #5

:       lda     (line),y
        sta     (backup_ptr),y
        ; draw palette
        lda     (sprite),y
        sta     (line),y
        dey
        bpl     :-

        clc
        ; Increment backup line
        lda     backup_ptr
        adc     #6
        sta     backup_ptr
        lda     backup_ptr+1
        adc     #0
        sta     backup_ptr+1

        ; Increment sprite line
        lda     sprite
        adc     #6
        sta     sprite
        lda     sprite+1
        adc     #0
        sta     sprite+1

        lda     cur_y
        adc     #2
        sta     cur_y

        dec     n_lines
        bpl     next_line

        rts

        .bss

palette_backup: .res 90
mouse_prev_x:   .res 1
mouse_prev_y:   .res 1
