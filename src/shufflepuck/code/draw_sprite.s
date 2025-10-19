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

        .export   _clear_sprite, _draw_sprite
        .export   _draw_sprite_big, _draw_sprite_big_set_coords
        .export   _skip_top_lines

        .export   n_bytes_draw, n_lines_draw
        .export   _draw_eor_my_pusher, _clear_eor_my_pusher

        .import   _hgr_hi, _hgr_low
        .import   _div7_table, _mod7_table

        .import   my_pusher_data

        .import   umul8x8r16
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
        sta     cur_sprite_ptr
        stx     cur_sprite_ptr+1

        ldy     #SPRITE_DATA::NEED_CLEAR
        lda     (cur_sprite_ptr),y
        beq     out               ; Skip clearing if not needed
        lda     #$00
        sta     (cur_sprite_ptr),y; Reset clear-needed flag

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

        ldy     #SPRITE_DATA::BG_BACKUP+1   ; Only update high byte as backups
        lda     (cur_sprite_ptr),y          ; are aligned
        sta     _clear_sprite::sprite_restore+2

        ; Now, clear

        ldx     n_bytes_draw
init_line:
        clc
x_coord:
        lda     #$FF
        ldy     cur_y
        cpy     #192
        bcs     out
        adc     _hgr_low,y        ; No need to clc, we just did
        sta     sprite_store_bg+1
        lda     _hgr_hi,y
        ;adc     #0 - carry won't be set here

clear_next_line:
        sta     sprite_store_bg+2

bytes_per_line:
        ldy     #$FF

next_byte:
        dex
sprite_restore:
        lda     $FF00,x
sprite_store_bg:
        sta     $FFFF,y
        dey
        bpl     next_byte

        cpx     #$00              ; Did we do all bytes? - sets carry
        beq     out

        inc     cur_y             ; Prepare next line

        lda     sprite_store_bg+2 ; Compute pointer to next line. It's 4 pages
        adc     #$04-1            ; down unless we're at the end of the range.
        cmp     #$40              ; Mind that carry was set so add 3.
        bcc     clear_next_line
        bcs     init_line         ; In which case reinit the full pointer.

out:
        rts
.endproc

.proc _draw_sprite
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

        ldy     #SPRITE_DATA::BG_BACKUP+1
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

        ; Flag for subsequent clear
        ldy     #SPRITE_DATA::NEED_CLEAR
        lda     #1
        sta     (cur_sprite_ptr),y

        ldx     n_bytes_draw      ; Get total number of bytes to draw
init_line:
        clc
x_coord:
        lda     #$FF              ; Patched by setup with top-left X coord
        ldy     cur_y
        cpy     #192              ; Avoid going out of screen
        bcs     out
        adc     _hgr_low,y        ; Get line address + X coord
        sta     sprite_get_bg+1
        sta     sprite_store_byte+1
        lda     _hgr_hi,y
;       adc     #0                ; Carry won't be set here, HGR lines don't cross

do_next_line:
        sta     sprite_get_bg+2
        sta     sprite_store_byte+2

bytes_per_line:
        ldy     #$FF              ; Get bytes to draw per line (patched by setup)

next_byte:
        dex
sprite_get_bg:
        lda     $FFFF,y           ; Get the background under the sprite
sprite_backup:
        sta     $FF00,x           ; Back it up for next clear
sprite_mask:
        and     $FFFF,x           ; AND background and sprite mask
sprite_pointer:
        ora     $FFFF,x           ; OR resulting with sprite data
sprite_store_byte:
        sta     $FFFF,y           ; Store on screen
        dey
        bpl     next_byte

        cpx     #$00              ; This sets carry!
        beq     out

        inc     cur_y

        lda     sprite_get_bg+2
        adc     #$04-1            ; carry is set
        cmp     #$40
        bcc     do_next_line
        bcs     init_line

out:    rts
.endproc

; A: how many lines to skip at the top of the sprite
; X: Sprite bytes per line
; Y: Total bytes
; Return new number of bytes in A
.proc _skip_top_lines
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

; A, X: sprite address
; Y: bytes per line
; Coords must be previously set by calling
; _draw_sprite_big_set_coords
.proc _draw_sprite_big
        sty     bytes_per_line+1
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

.proc _draw_sprite_big_set_coords
        stx     _draw_sprite_big::x_coord+1
        sty     cur_y
        sta     n_lines_draw
        rts
.endproc

; Only our pusher is drawn via _eor, so take shortcuts
.proc _draw_eor_my_pusher
        ; Draw setup
        lda     my_pusher_data+SPRITE_DATA::Y_COORD
        sta     cur_y
        sta     my_pusher_data+SPRITE_DATA::PREV_Y_COORD  ; Save the new prev_y

        ldx     my_pusher_data+SPRITE_DATA::X_COORD
        lda     _div7_table,x                             ; Compute divided X
        sta     _draw_eor_my_pusher::x_coord+1
        sta     my_pusher_data+SPRITE_DATA::PREV_X_COORD  ; And save the new prev_x, divided by 7, now

        lda     my_pusher_data+SPRITE_DATA::SPRITE        ; Get pointer to sprite
        sta     ptr2
        lda     my_pusher_data+SPRITE_DATA::SPRITE+1
        sta     ptr2+1

        ; Select correct sprite for pixel-precise render
        lda     _mod7_table,x
        asl                                               ; Multiply by two because pointer
        tay
        lda     (ptr2),y
        sta     _draw_eor_my_pusher::sprite_pointer+1
        sta     my_pusher_data+SPRITE_DATA::BG_BACKUP     ; And save the sprite to clear (high byte)
        iny
        lda     (ptr2),y
        sta     _draw_eor_my_pusher::sprite_pointer+2
        sta     my_pusher_data+SPRITE_DATA::BG_BACKUP+1   ; And save the sprite to clear (low byte)

        ; We're going to draw, set clear-needed flag
        inc     my_pusher_data+SPRITE_DATA::NEED_CLEAR

draw:                                                     ; Also used for clearing
        ldx     my_pusher_data+SPRITE_DATA::BYTES         ; Total number of bytes to draw

        lda     my_pusher_data+SPRITE_DATA::BYTES_WIDTH   ; number of bytes per line (-1)
        sta     _draw_eor_my_pusher::bytes_per_line+1

init_line:
        clc
x_coord:
        lda     #$FF              ; Patched by setup with top-left X coord (in bytes)
        ldy     cur_y
        adc     _hgr_low,y        ; Get HGR line Y address plus X bytes
        sta     load_background+1 ; Patch loop
        sta     store_byte+1

        lda     _hgr_hi,y         ; Same for high byte
;       adc     #0                ; Carry won't be set here

do_next_line:
        sta     load_background+2
        sta     store_byte+2

bytes_per_line:
        ldy     #$FF

next_byte:
        dex
load_background:
        lda     $FFFF,y       ; Get what's under the sprite
sprite_pointer:
        eor     $FFFF,x       ; EOR with sprite byte
store_byte:
        sta     $FFFF,y       ; Store on-screen
        dey
        bpl     next_byte

        cpx     #$00          ; End of line, are there more lines? (cpx sets carry!)
        beq     out

        inc     cur_y

        lda     load_background+2
        adc     #$04-1        ; Carry is set
        cmp     #$40
        bcc     do_next_line
        bcs     init_line

out:    rts
.endproc

.proc _clear_eor_my_pusher
        lda     my_pusher_data+SPRITE_DATA::NEED_CLEAR
        beq     out                                     ; Skip clearing if not needed
        lda     #$00
        sta     my_pusher_data+SPRITE_DATA::NEED_CLEAR  ; Reset clear-needed flag

        lda     my_pusher_data+SPRITE_DATA::PREV_Y_COORD; Get existing prev_y for clear
        sta     cur_y

        lda     my_pusher_data+SPRITE_DATA::PREV_X_COORD; Already divided
        sta     _draw_eor_my_pusher::x_coord+1

        lda     my_pusher_data+SPRITE_DATA::BG_BACKUP   ; Restore the sprite we need to clear
        sta     _draw_eor_my_pusher::sprite_pointer+1 ; It may have been a different sized one
        lda     my_pusher_data+SPRITE_DATA::BG_BACKUP+1
        sta     _draw_eor_my_pusher::sprite_pointer+2

        jmp     _draw_eor_my_pusher::draw
out:    rts
.endproc

; Kept for reference - general version with a pointer to sprite data
; .proc _draw_eor
;         ; Draw setup
;         sta     cur_sprite_ptr
;         stx     cur_sprite_ptr+1
;         ldy     #SPRITE_DATA::X_COORD
;         lda     (cur_sprite_ptr),y
;         tax
; 
;         ldy     #SPRITE_DATA::Y_COORD
;         lda     (cur_sprite_ptr),y
;         sta     cur_y
; 
;         ldy     #SPRITE_DATA::PREV_Y_COORD
;         sta     (cur_sprite_ptr),y          ; Save the new prev_y
; 
;         lda     _div7_table,x               ; Compute divided X
;         sta     _draw_eor::x_coord+1
; 
;         ldy     #SPRITE_DATA::PREV_X_COORD
;         sta     (cur_sprite_ptr),y          ; And save the new prev_x, divided by 7, now
; 
;         ldy     #SPRITE_DATA::SPRITE
;         lda     (cur_sprite_ptr),y
;         sta     ptr2
;         iny
;         lda     (cur_sprite_ptr),y
;         sta     ptr2+1
; 
;         ; Select correct sprite for pixel-precise render
;         lda     _mod7_table,x
;         asl                       ; Multiply by two because pointer
;         tay
;         lda     (ptr2),y
;         sta     _draw_eor::sprite_pointer+1
;         tax
;         iny
;         lda     (ptr2),y
;         sta     _draw_eor::sprite_pointer+2
; 
;         ldy     #SPRITE_DATA::BG_BACKUP+1
;         sta     (cur_sprite_ptr),y          ; And save the sprite to clear
;         dey
;         txa
;         sta     (cur_sprite_ptr),y
; 
;         ; We're going to draw, set clear-needed flag
;         ldy     #SPRITE_DATA::NEED_CLEAR
;         lda     #1
;         sta     (cur_sprite_ptr),y
; 
; draw:                             ; Also used for clearing
;         ldy     #SPRITE_DATA::BYTES
;         lda     (cur_sprite_ptr),y
;         tax                       ; Total number of bytes to draw
; 
;         ldy     #SPRITE_DATA::BYTES_WIDTH
;         lda     (cur_sprite_ptr),y
;         sta     _draw_eor::bytes_per_line+1
; 
; init_line:
;         clc
; x_coord:
;         lda     #$FF              ; Patched by setup with top-left X coord (in bytes)
;         ldy     cur_y
;         adc     _hgr_low,y        ; Get HGR line Y address plus X bytes
;         sta     load_background+1 ; Patch loop
;         sta     store_byte+1
; 
;         lda     _hgr_hi,y         ; Same for high byte
; ;       adc     #0                ; Carry won't be set here
; 
; do_next_line:
;         sta     load_background+2
;         sta     store_byte+2
; 
; bytes_per_line:
;         ldy     #$FF
; 
; next_byte:
;         dex
; load_background:
;         lda     $FFFF,y       ; Get what's under the sprite
; sprite_pointer:
;         eor     $FFFF,x       ; Patched by setup
; store_byte:
;         sta     $FFFF,y       ; Store on-screen
;         dey
;         bpl     next_byte
; 
;         cpx     #$00              ; This sets carry!
;         beq     out
; 
;         inc     cur_y
; 
;         lda     load_background+2
;         adc     #$04-1            ; Carry is set
;         cmp     #$40
;         bcc     do_next_line
;         bcs     init_line
; 
; out:    rts
; .endproc
; 
; .proc _clear_eor
;         sta     cur_sprite_ptr
;         stx     cur_sprite_ptr+1
; 
;         ldy     #SPRITE_DATA::NEED_CLEAR
;         lda     (cur_sprite_ptr),y
;         beq     out               ; Skip clearing if not needed
;         lda     #$00
;         sta     (cur_sprite_ptr),y; Reset clear-needed flag
; 
;         ldy     #SPRITE_DATA::PREV_Y_COORD
;         lda     (cur_sprite_ptr),y          ; Get existing prev_y for clear
;         sta     cur_y
; 
;         ldy     #SPRITE_DATA::PREV_X_COORD
;         lda     (cur_sprite_ptr),y
;         sta     _draw_eor::x_coord+1
; 
;         ldy     #SPRITE_DATA::BG_BACKUP     ; Restore the sprite we need to clear
;         lda     (cur_sprite_ptr),y          ; It may have been a different sized one
;         sta     _draw_eor::sprite_pointer+1
;         iny
;         lda     (cur_sprite_ptr),y
;         sta     _draw_eor::sprite_pointer+2
; 
;         jmp     _draw_eor::draw
; out:    rts
; .endproc
