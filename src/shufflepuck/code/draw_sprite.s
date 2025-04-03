; Copyright (C) 2025 Colin Leroy-Mira <colin@colino.net>
;
; This program is free software; you can redistribute it and/or modify
; it under the terms of the GNU General Public License as published by
; the Free Software Foundation; either version 3 of the License, or
; (at your option) any later version.
;
; This program is distributed in the hope that it will be useful,
; but WITHOUT ANY WARRANTY; without even the implied warranty of
; MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
; GNU General Public License for more details.
;
; You should have received a copy of the GNU General Public License
; along with this program. If not, see <http://www.gnu.org/licenses/>.

        .export   _clear_sprite, _draw_sprite, _draw_sprite_big
        .export   _load_puck_pointer, _load_my_pusher_pointer, _load_their_pusher_pointer
        .export   _setup_sprite_pointer_for_clear
        .export   _setup_sprite_pointer_for_draw

        .export   big_sprite_pointer
        .export   big_n_bytes_per_line_draw, big_sprite_x
        .export   sprite_y, n_bytes_draw, n_lines_draw

        .import   puck_data, my_pusher_data, their_pusher_data

        .importzp _zp8p, _zp10, _zp11
        .importzp tmp4, ptr2
        .import   _hgr_hi, _hgr_low
        .import   _div7_table, _mod7_table

        .include "apple2.inc"
        .include "puck0.gen.inc"
        .include "my_pusher0.gen.inc"
        .include "their_pusher4.gen.inc"
        .include "sprite.inc"
        .include "zp_vars.inc"

.segment "LOWCODE"

line            = _zp8p
cur_y           = _zp10
n_bytes_draw    = _zp11
n_lines_draw    = _zp11 ; shared
sprite_y        = tmp4

_load_puck_pointer:
        lda     #<puck_data
        sta     cur_sprite_ptr
        lda     #>puck_data
        sta     cur_sprite_ptr+1

        rts

_load_my_pusher_pointer:
        lda     #<my_pusher_data
        sta     cur_sprite_ptr
        lda     #>my_pusher_data
        sta     cur_sprite_ptr+1

        rts

_load_their_pusher_pointer:
        lda     #<their_pusher_data
        sta     cur_sprite_ptr
        lda     #>their_pusher_data
        sta     cur_sprite_ptr+1

        rts

; Finish setting up clear/draw functions with sprite data
_setup_sprite_pointer_for_clear:
        ldy     #SPRITE_DATA::PREV_X_COORD
        lda     (cur_sprite_ptr),y
        sta     sprite_prev_x+1

        ldy     #SPRITE_DATA::PREV_Y_COORD
        lda     (cur_sprite_ptr),y          ; Get existing prev_y for clear
        sta     cur_y

        ldy     #SPRITE_DATA::BYTES
        lda     (cur_sprite_ptr),y
        sta     n_bytes_draw

        ldy     #SPRITE_DATA::BYTES_WIDTH
        lda     (cur_sprite_ptr),y
        sta     n_bytes_per_line_clear+1

        ldy     #SPRITE_DATA::BG_BACKUP
        lda     (cur_sprite_ptr),y
        sta     sprite_restore+1
        iny
        lda     (cur_sprite_ptr),y
        sta     sprite_restore+2
        rts

_setup_sprite_pointer_for_draw:
        ldy     #SPRITE_DATA::X_COORD
        lda     (cur_sprite_ptr),y
        tax

        ; Select correct sprite for pixel-precise render
        lda     _mod7_table,x
        asl                       ; Multiply by four to account for
        asl                       ; data and mask pointers
        sta     sprite_num+1      ; Patch sprite pointer number

        lda     _div7_table,x     ; Compute divided X
        sta     sprite_x+1

        ldy     #SPRITE_DATA::PREV_X_COORD
        sta     (cur_sprite_ptr),y          ; And save the new prev_x now

        ldy     #SPRITE_DATA::Y_COORD
        lda     (cur_sprite_ptr),y
        sta     sprite_y


        ldy     #SPRITE_DATA::PREV_Y_COORD
        sta     (cur_sprite_ptr),y          ; Save the new prev_y

        ldy     #SPRITE_DATA::BYTES
        lda     (cur_sprite_ptr),y
        sta     n_bytes_draw

        ldy     #SPRITE_DATA::BYTES_WIDTH
        lda     (cur_sprite_ptr),y
        sta     n_bytes_per_line_draw+1

        ldy     #SPRITE_DATA::BG_BACKUP
        lda     (cur_sprite_ptr),y
        sta     sprite_backup+1
        iny
        lda     (cur_sprite_ptr),y
        sta     sprite_backup+2

select_sprite:
        ldy     #SPRITE_DATA::SPRITE
        lda     (cur_sprite_ptr),y
        sta     ptr2
        iny
        lda     (cur_sprite_ptr),y
        sta     ptr2+1

sprite_num:
        ldy     #$FF
        lda     (ptr2),y
        sta     sprite_pointer+1
        iny
        lda     (ptr2),y
        sta     sprite_pointer+2
        iny
        lda     (ptr2),y
        sta     sprite_mask+1
        iny
        lda     (ptr2),y
        sta     sprite_mask+2
        rts

; X, Y : coordinates
_clear_sprite:
        ldy     #SPRITE_DATA::NEED_CLEAR
        lda     (cur_sprite_ptr),y
        bne     :+                ; Skip clearing if not needed
        rts
:       lda     #0
        sta     (cur_sprite_ptr),y; Reset clear-needed flag

        ldx     n_bytes_draw
        clc
clear_next_line:
sprite_prev_x:
        lda     #$FF
        ldy     cur_y
        adc     _hgr_low,y
        sta     sprite_store_bg+1
        lda     _hgr_hi,y
        ;adc     #0 - carry won't be set here
        sta     sprite_store_bg+2

n_bytes_per_line_clear:
        ldy     #$FF

sprite_restore:
        lda     $FFFF,x
sprite_store_bg:
        sta     $FFFF,y
        dex
        dey
        bpl     sprite_restore    ; Next byte

        inc     cur_y
        cpx     #$FF              ; Did we do all bytes?
        bne     clear_next_line
        rts

_draw_sprite:
        ; Clear done, now draw
        ldy     #SPRITE_DATA::NEED_CLEAR
        lda     #1
        sta     (cur_sprite_ptr),y

        ldy     sprite_y
        sty     cur_y

        ldx     n_bytes_draw
        clc
next_line:
sprite_x:
        lda     #$FF
        ldy     cur_y
        adc     _hgr_low,y
        sta     sprite_get_bg+1
        sta     sprite_store_byte+1
        lda     _hgr_hi,y
        ;adc     #0 - carry won't be set here
        sta     sprite_get_bg+2
        sta     sprite_store_byte+2

n_bytes_per_line_draw:
        ldy     #$FF

        ; Get what's under the sprite
sprite_get_bg:
        lda     $FFFF,y
sprite_backup:
        ; Back it up
        sta     $FFFF,x
        ; draw sprite
sprite_mask:
        and     $FFFF,x       ; Patched
sprite_pointer:
        ora     $FFFF,x       ; Patched
sprite_store_byte:
        sta     $FFFF,y
        dex
        dey
        bpl     sprite_get_bg ; Next byte

        inc     cur_y
        cpx     #$FF
        bne     next_line
draw_out:
        rts

_draw_sprite_big:
        stx     big_sprite_x+1
        sty     sprite_y
        sty     cur_y

big_next_line:
big_sprite_x:
        lda     #$FF
        clc
        ldy     cur_y
        adc     _hgr_low,y
        sta     line
        lda     _hgr_hi,y
        ; adc     #0 - carry won't be set here
        sta     line+1

big_n_bytes_per_line_draw:
        ldy     #$FF
        dey
big_sprite_pointer:
:       lda     $FFFF,y       ; Patched
        sta     (line),y      ; Use ($nn),y
        dey
        bpl     :-

        dec     cur_y
        lda     big_sprite_pointer+1
        clc
        adc     big_n_bytes_per_line_draw+1
        sta     big_sprite_pointer+1
        lda     big_sprite_pointer+2
        adc     #0
        sta     big_sprite_pointer+2

        dec     n_lines_draw
        bpl     big_next_line

        rts
