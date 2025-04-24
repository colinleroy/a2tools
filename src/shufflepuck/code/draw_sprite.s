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
        .export   _setup_sprite_pointer_for_clear
        .export   _setup_sprite_pointer_for_draw
        .export   _skip_top_lines

        .export   n_bytes_draw, n_lines_draw
        .export   _setup_eor_draw, _setup_eor_clear, _draw_eor, _clear_eor
        
        .import   _hgr_hi, _hgr_low
        .import   _div7_table, _mod7_table

        .import   popax, umul8x8r16
        .importzp ptr2, ptr1
        .importzp _zp8p, _zp10, _zp11

        .include  "apple2.inc"
        .include  "puck0.gen.inc"
        .include  "my_pusher0.gen.inc"
        .include  "their_pusher4.gen.inc"
        .include  "sprite.inc"
        .include  "zp_vars.inc"

.segment "LOWCODE"

line            = _zp8p
cur_y           = _zp10
n_bytes_draw    = _zp11
n_lines_draw    = _zp11 ; shared

; X, Y : coordinates
.proc _clear_sprite
        ldy     #SPRITE_DATA::NEED_CLEAR
        lda     (cur_sprite_ptr),y
        bne     :+                ; Skip clearing if not needed
        rts
:       lda     #0
        sta     (cur_sprite_ptr),y; Reset clear-needed flag

        ldx     n_bytes_draw
        clc
clear_next_line:
x_coord:
        lda     #$FF
        ldy     cur_y
        cpy     #192
        bcs     out
        adc     _hgr_low,y
        sta     sprite_store_bg+1
        lda     _hgr_hi,y
        ;adc     #0 - carry won't be set here
        sta     sprite_store_bg+2

bytes_per_line:
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
out:
        rts
.endproc

.proc _draw_sprite
        ; Clear done, now draw
        ldy     #SPRITE_DATA::NEED_CLEAR
        lda     #1
        sta     (cur_sprite_ptr),y

        ldx     n_bytes_draw
        clc
next_line:
x_coord:
        lda     #$FF
        ldy     cur_y
        cpy     #192
        bcs     out
        adc     _hgr_low,y
        sta     sprite_get_bg+1
        sta     sprite_store_byte+1
        lda     _hgr_hi,y
        ;adc     #0 - carry won't be set here
        sta     sprite_get_bg+2
        sta     sprite_store_byte+2

bytes_per_line:
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

out:    rts
.endproc

; Finish setting up clear/draw functions with sprite data
.proc _setup_sprite_pointer_for_clear
        sta     cur_sprite_ptr
        stx     cur_sprite_ptr+1
        ldy     #SPRITE_DATA::PREV_X_COORD
        lda     (cur_sprite_ptr),y
        sta     _clear_sprite::x_coord+1

        ldy     #SPRITE_DATA::PREV_Y_COORD
        lda     (cur_sprite_ptr),y          ; Get existing prev_y for clear
        sta     cur_y

        ldy     #SPRITE_DATA::BYTES
        lda     (cur_sprite_ptr),y
        sta     n_bytes_draw

        ldy     #SPRITE_DATA::BYTES_WIDTH
        lda     (cur_sprite_ptr),y
        sta     _clear_sprite::bytes_per_line+1

        ldy     #SPRITE_DATA::BG_BACKUP
        lda     (cur_sprite_ptr),y
        sta     _clear_sprite::sprite_restore+1
        iny
        lda     (cur_sprite_ptr),y
        sta     _clear_sprite::sprite_restore+2
        rts
.endproc

.proc _setup_sprite_pointer_for_draw
        sta     cur_sprite_ptr
        stx     cur_sprite_ptr+1
        ldy     #SPRITE_DATA::X_COORD
        lda     (cur_sprite_ptr),y
        tax

        ldy     #SPRITE_DATA::Y_COORD
        lda     (cur_sprite_ptr),y
        sta     cur_y

        ldy     #SPRITE_DATA::PREV_Y_COORD
        sta     (cur_sprite_ptr),y          ; Save the new prev_y

        lda     _div7_table,x               ; Compute divided X
        sta     _draw_sprite::x_coord+1

        ldy     #SPRITE_DATA::PREV_X_COORD
        sta     (cur_sprite_ptr),y          ; And save the new prev_x now

        ldy     #SPRITE_DATA::BYTES
        lda     (cur_sprite_ptr),y
        sta     n_bytes_draw

        ldy     #SPRITE_DATA::BYTES_WIDTH
        lda     (cur_sprite_ptr),y
        sta     _draw_sprite::bytes_per_line+1

        ldy     #SPRITE_DATA::BG_BACKUP
        lda     (cur_sprite_ptr),y
        sta     _draw_sprite::sprite_backup+1
        iny
        lda     (cur_sprite_ptr),y
        sta     _draw_sprite::sprite_backup+2

select_sprite:
        ldy     #SPRITE_DATA::SPRITE
        lda     (cur_sprite_ptr),y
        sta     ptr2
        iny
        lda     (cur_sprite_ptr),y
        sta     ptr2+1

        ; Select correct sprite for pixel-precise render
        lda     _mod7_table,x
        asl                       ; Multiply by four to account for
        asl                       ; data and mask pointers
        tay

        lda     (ptr2),y
        sta     _draw_sprite::sprite_pointer+1
        iny
        lda     (ptr2),y
        sta     _draw_sprite::sprite_pointer+2
        iny
        lda     (ptr2),y
        sta     _draw_sprite::sprite_mask+1
        iny
        lda     (ptr2),y
        sta     _draw_sprite::sprite_mask+2
        rts
.endproc

; A: how many lines to skip at the top of the sprite
; X: Sprite bytes per line
; Y: Total bytes
; Return new number of bytes in A
.proc _skip_top_lines
        dey
        sty     total_bytes+1
        stx     ptr1
        jsr     umul8x8r16
        sta     ptr1
total_bytes:
        lda     #$FF
        sec
        sbc     ptr1
        rts
.endproc

; X, Y: coords
; A: bytes per line
; TOS: sprite pointer
.proc _draw_sprite_big
        stx     x_coord+1
        sty     cur_y
        sta     bytes_per_line+1
        jsr     popax
        sta     sprite_pointer+1
        stx     sprite_pointer+2

next_line:
x_coord:
        lda     #$FF
        clc
        ldy     cur_y
        adc     _hgr_low,y
        sta     line
        lda     _hgr_hi,y
        ; adc     #0 - carry won't be set here
        sta     line+1

bytes_per_line:
        ldy     #$FF
        dey
sprite_pointer:
:       lda     $FFFF,y       ; Patched
        sta     (line),y      ; Use ($nn),y
        dey
        bpl     :-

        dec     cur_y
        lda     sprite_pointer+1
        clc
        adc     bytes_per_line+1
        sta     sprite_pointer+1
        lda     sprite_pointer+2
        adc     #0
        sta     sprite_pointer+2

        dec     n_lines_draw
        bne     next_line

        rts
.endproc

.proc _draw_eor
        ldy     #SPRITE_DATA::NEED_CLEAR
        lda     #1
        sta     (cur_sprite_ptr),y

draw:
        ldx     n_bytes_draw
        clc
next_line:
x_coord:
        lda     #$FF
        ldy     cur_y
        adc     _hgr_low,y
        sta     load_background+1
        sta     store_byte+1
        lda     _hgr_hi,y
        ;adc     #0 - carry won't be set here
        sta     load_background+2
        sta     store_byte+2

bytes_per_line:
        ldy     #$FF

        ; Get what's under the sprite
load_background:
        lda     $FFFF,y
sprite_pointer:
        eor     $FFFF,x       ; Patched
store_byte:
        sta     $FFFF,y
        dex
        dey
        bpl     load_background ; Next byte

        inc     cur_y
        cpx     #$FF
        bne     next_line
        rts
.endproc

.proc _clear_eor
        ldy     #SPRITE_DATA::NEED_CLEAR
        lda     (cur_sprite_ptr),y
        bne     :+                ; Skip clearing if not needed
        rts
:       lda     #0
        sta     (cur_sprite_ptr),y; Reset clear-needed flag
        jmp     _draw_eor::draw
.endproc

.proc _setup_eor_clear
        sta     cur_sprite_ptr
        stx     cur_sprite_ptr+1
        ldy     #SPRITE_DATA::PREV_Y_COORD
        lda     (cur_sprite_ptr),y          ; Get existing prev_y for clear
        sta     cur_y

        ldy     #SPRITE_DATA::PREV_X_COORD
        lda     (cur_sprite_ptr),y
        sta     _draw_eor::x_coord+1

        ldy     #SPRITE_DATA::BG_BACKUP     ; Restore the sprite we need to clear
        lda     (cur_sprite_ptr),y
        sta     _draw_eor::sprite_pointer+1
        iny
        lda     (cur_sprite_ptr),y
        sta     _draw_eor::sprite_pointer+2
        jmp     _setup_eor_common
.endproc

.proc _setup_eor_draw
        sta     cur_sprite_ptr
        stx     cur_sprite_ptr+1
        ldy     #SPRITE_DATA::X_COORD
        lda     (cur_sprite_ptr),y
        tax

        ldy     #SPRITE_DATA::Y_COORD
        lda     (cur_sprite_ptr),y
        sta     cur_y

        ldy     #SPRITE_DATA::PREV_Y_COORD
        sta     (cur_sprite_ptr),y          ; Save the new prev_y

        lda     _div7_table,x               ; Compute divided X
        sta     _draw_eor::x_coord+1

        ldy     #SPRITE_DATA::PREV_X_COORD
        sta     (cur_sprite_ptr),y          ; And save the new prev_x, divided by 7, now

        ldy     #SPRITE_DATA::SPRITE
        lda     (cur_sprite_ptr),y
        sta     ptr2
        iny
        lda     (cur_sprite_ptr),y
        sta     ptr2+1

        ; Select correct sprite for pixel-precise render
        lda     _mod7_table,x
        asl                       ; Multiply by two because pointer
        tay
        lda     (ptr2),y
        sta     _draw_eor::sprite_pointer+1
        tax
        iny
        lda     (ptr2),y
        sta     _draw_eor::sprite_pointer+2

        ldy     #SPRITE_DATA::BG_BACKUP+1
        sta     (cur_sprite_ptr),y          ; And save the sprite to clear
        dey
        txa
        sta     (cur_sprite_ptr),y
        ; Fallthrough to setup_eor_common
.endproc

.proc _setup_eor_common
        ldy     #SPRITE_DATA::BYTES
        lda     (cur_sprite_ptr),y
        sta     n_bytes_draw

        ldy     #SPRITE_DATA::BYTES_WIDTH
        lda     (cur_sprite_ptr),y
        sta     _draw_eor::bytes_per_line+1
        rts
.endproc
