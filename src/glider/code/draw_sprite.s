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

        .export _draw_sprite, _draw_sprite_fast
        .export _load_sprite_pointer, _setup_sprite_pointer

        .export fast_sprite_pointer
        .export fast_n_bytes_per_line_draw, fast_sprite_x
        .export sprite_y, n_bytes_draw

        .importzp _zp8p, _zp10, _zp11
        .importzp tmp4, ptr2
        .import   _hgr_hi, _hgr_low
        .import   _div7_table, _mod7_table

        .include "apple2.inc"
        .include "plane.gen.inc"
        .include "rubber_band_reverted.gen.inc"
        .include "sprite.inc"
        .include "level_data_ptr.inc"

.segment "LOWCODE"

line            = _zp8p
cur_y           = _zp10
n_bytes_draw    = _zp11

sprite_y        = tmp4

; pointer to sprite data in (level_data), A is sprite number to draw
; Return with 1 in A if the sprite is static
_load_sprite_pointer:
        asl
        tay
        lda     (level_data),y
        sta     cur_sprite_ptr
        iny
        lda     (level_data),y
        sta     cur_sprite_ptr+1

        ldy     #SPRITE_DATA::STATIC
        lda     (cur_sprite_ptr),y

        rts

; Finish setting up clear/draw functions with sprite data
_setup_sprite_pointer:
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
        tax                       ; Back it up to update prev X

        ldy     #SPRITE_DATA::PREV_X_COORD
        lda     (cur_sprite_ptr),y
        sta     sprite_prev_x+1

        txa
        sta     (cur_sprite_ptr),y          ; And save the new prev_x now

        ldy     #SPRITE_DATA::Y_COORD
        lda     (cur_sprite_ptr),y
        sta     sprite_y
        tax

        ldy     #SPRITE_DATA::PREV_Y_COORD
        lda     (cur_sprite_ptr),y          ; Get existing prev_y for clear
        sta     cur_y
        txa
        sta     (cur_sprite_ptr),y          ; Save the new prev_y

        ldy     #SPRITE_DATA::BYTES
        lda     (cur_sprite_ptr),y
        sta     n_bytes_draw

        ldy     #SPRITE_DATA::BYTES_WIDTH
        lda     (cur_sprite_ptr),y
        sta     n_bytes_per_line_clear+1
        sta     n_bytes_per_line_draw+1

        ldy     #SPRITE_DATA::BG_BACKUP
        lda     (cur_sprite_ptr),y
        sta     sprite_restore+1
        sta     sprite_backup+1
        iny
        lda     (cur_sprite_ptr),y
        sta     sprite_restore+2
        sta     sprite_backup+2

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
_draw_sprite:
        ldy     #SPRITE_DATA::NEED_CLEAR
        lda     (cur_sprite_ptr),y
        beq     blit_sprite       ; Skip clearing if not needed
        lda     #0
        sta     (cur_sprite_ptr),y; Reset clear-needed flag

        ldx     n_bytes_draw

clear_init_line:
        clc
sprite_prev_x:
        lda     #$FF
        ldy     cur_y
        adc     _hgr_low,y
        sta     sprite_store_bg+1
        lda     _hgr_hi,y
        ;adc     #0 - carry won't be set here

clear_next_line:
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
        beq     maybe_blit        ; Yes

        lda     sprite_store_bg+2 ; Consider easy case where the next HGR line
        adc     #$04              ; is 4 pages after this one, which is true
        cmp     #$40              ; until we reached the bottom of the screen
        bcc     clear_next_line   
        bcs     clear_init_line

maybe_blit:
        ; Don't draw if sprite not active
        ldy     #SPRITE_DATA::ACTIVE
        lda     (cur_sprite_ptr),y
        beq     out

blit_sprite:
        ; Clear done, now draw
        ldy     #SPRITE_DATA::NEED_CLEAR
        lda     #1
        sta     (cur_sprite_ptr),y

        ldy     sprite_y
        sty     cur_y

        ldx     n_bytes_draw

init_line:
        clc
sprite_x:
        lda     #$FF              ; Patched by setup with top-left X coord
        ldy     cur_y
        adc     _hgr_low,y        ; Get line address + X coord
        sta     sprite_get_bg+1
        sta     sprite_store_byte+1
        lda     _hgr_hi,y
;       adc     #0                ; Carry won't be set here, HGR lines don't cross

do_next_line:
        sta     sprite_get_bg+2
        sta     sprite_store_byte+2

n_bytes_per_line_draw:
        ldy     #$FF              ; Get bytes to draw per line (patched by setup)

sprite_get_bg:
        lda     $FFFF,y           ; Get the background under the sprite
sprite_backup:
        sta     $FFFF,x           ; Back it up for next clear
sprite_mask:
        and     $FFFF,x           ; AND background and sprite mask
sprite_pointer:
        ora     $FFFF,x           ; OR resulting with sprite data
sprite_store_byte:
        sta     $FFFF,y           ; Store on screen
        dex
        dey
        bpl     sprite_get_bg     ; Next byte

        inc     cur_y
        cpx     #$FF
        beq     out

        lda     sprite_get_bg+2
        adc     #$04
        cmp     #$40
        bcc     do_next_line
        bcs     init_line

out:    rts

_draw_sprite_fast:
        ldy     sprite_y
        sty     cur_y

        ldx     n_bytes_draw
        clc
fast_next_line:
fast_sprite_x:
        lda     #$FF
        ldy     cur_y
        adc     _hgr_low,y
        sta     line
        lda     _hgr_hi,y
        ; adc     #0 - carry won't be set here
        sta     line+1

fast_n_bytes_per_line_draw:
        ldy     #$FF

fast_sprite_pointer:
:       lda     $FFFF,x       ; Patched
        sta     (line),y      ; Use ($nn),y here because fast-drawn sprites
        dex                   ; are usually 1 byte large
        dey
        bpl     :-

        inc     cur_y
        cpx     #$FF
        bne     fast_next_line

        rts
