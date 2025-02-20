        .export _draw_sprite, _clear_and_draw_sprite
        .export _setup_sprite_pointer

        .importzp _zp6p, _zp8, _zp9, _zp10, _zp11, _zp12
        .importzp tmp1, tmp2, tmp3, tmp4, ptr1, ptr2, ptr3, ptr4
        .import   _hgr_hi, _hgr_low
        .import   _div7_table, _mod7_table

        .import   sprite_data

        .include "apple2.inc"
        .include "plane.inc"
        .include "sprite.inc"
        .include "level_data_ptr.inc"

line       = _zp6p
cur_y      = _zp9
n_bytes    = _zp11

sprite_y      = tmp4

; pointer to sprite data in (level_data), A is sprite number to draw
_setup_sprite_pointer:
        asl
        tay
        lda     (level_data),y
        sta     ptr1
        iny
        lda     (level_data),y
        sta     ptr1+1

        ldy     #SPRITE_DATA::X_COORD
        lda     (ptr1),y
        tax

        ; Select correct sprite for pixel-precise render
        lda     _mod7_table,x
        asl
        sta     sprite_num+1      ; Patch sprite pointer

        lda     _div7_table,x     ; Compute divided X
        sta     sprite_x+1
        tax                       ; Back it up to update prev X

        ldy     #SPRITE_DATA::PREV_X_COORD
        lda     (ptr1),y
        sta     sprite_prev_x+1

        txa
        sta     (ptr1),y          ; And save the new prev_x now

        ldy     #SPRITE_DATA::Y_COORD
        lda     (ptr1),y
        sta     sprite_y
        tax

        ldy     #SPRITE_DATA::PREV_Y_COORD
        lda     (ptr1),y          ; Get existing prev_y for clear
        sta     cur_y
        txa
        sta     (ptr1),y          ; Save the new prev_y

        ldy     #SPRITE_DATA::BYTES
        lda     (ptr1),y
        sta     n_bytes

        ldy     #SPRITE_DATA::WIDTH
        lda     (ptr1),y
        sta     n_bytes_per_line_1+1
        sta     n_bytes_per_line_2+1

        ldy     #SPRITE_DATA::BACKUP_BUFFER
        lda     (ptr1),y
        sta     sprite_backup_1+1
        sta     sprite_backup_2+1
        iny
        lda     (ptr1),y
        sta     sprite_backup_1+2
        sta     sprite_backup_2+2

        ldy     #SPRITE_DATA::SPRITE
        lda     (ptr1),y
        sta     ptr2
        iny
        lda     (ptr1),y
        sta     ptr2+1

        ldy     #SPRITE_DATA::SPRITE_MASK
        lda     (ptr1),y
        sta     ptr3
        iny
        lda     (ptr1),y
        sta     ptr3+1

sprite_num:
        ldy     #$FF
        lda     (ptr2),y
        sta     sprite_pointer+1
        lda     (ptr3),y
        sta     sprite_mask+1

        iny
        lda     (ptr2),y
        sta     sprite_pointer+2
        lda     (ptr3),y
        sta     sprite_mask+2

        rts

; X, Y : coordinates
_clear_and_draw_sprite:
        ldx     n_bytes
        clc
clear_next_line:
sprite_prev_x:
        lda     #$FF
        ldy     cur_y
        adc     _hgr_low,y
        sta     line
        lda     _hgr_hi,y
        adc     #0
        sta     line+1

n_bytes_per_line_1:
        ldy     #$FF

sprite_backup_1:
:       lda     $FFFF,x
        sta     (line),y
        dex
        dey
        bpl     :-

        inc     cur_y
        cpx     #$FF              ; Did we do all bytes?
        bne     clear_next_line

_draw_sprite:
        ldy     sprite_y
        sty     cur_y

        ldx     n_bytes
        clc
next_line:
sprite_x:
        lda     #$FF
        ldy     cur_y
        adc     _hgr_low,y
        sta     line
        lda     _hgr_hi,y
        adc     #0
        sta     line+1

n_bytes_per_line_2:
        ldy     #$FF

:       lda     (line),y
sprite_backup_2:
        sta     $FFFF,x
        ; draw sprite
sprite_mask:
        and     $FFFF,x       ; Patched
sprite_pointer:
        ora     $FFFF,x       ; Patched
        sta     (line),y
        dex
        dey
        bpl     :-

        inc     cur_y
        cpx     #$FF
        bne     next_line

        rts
