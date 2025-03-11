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

                  .export  _animate_plane_crash

                  .import  _plane, plane_data
                  .import  _plane_destroyed

                  .import  _draw_sprite
                  .import  _setup_sprite_pointer, _load_sprite_pointer
                  .import  plane_sprite_num
                  .import  _div7_table, tosumula0, pusha0, pushax, popax
                  .import  _platform_msleep

                  .importzp tmp1, tmp2

                  .include "level_data_ptr.inc"
                  .include "sprite.inc"

.segment "LOWCODE"

; Sprite number in A, sprite in TOS
.proc _replace_sprite
        jsr     _load_sprite_pointer

        jsr     popax
        ldy     #SPRITE_DATA::SPRITE
        sta     (cur_sprite_ptr),y
        iny
        txa
        sta     (cur_sprite_ptr),y

        rts
.endproc

; Round a number to the closest lower multiple of seven
.proc _round_to_seven
        tax
        lda     _div7_table,x
        jsr     pusha0
        lda     #7
        jmp     tosumula0
.endproc

.proc _animate_plane_crash
        ; Update X coord to make sure we can play the animation in order
        lda     plane_data+SPRITE_DATA::X_COORD
        jsr     _round_to_seven
        sta     plane_data+SPRITE_DATA::X_COORD

        ; Get the plane's sprite number (last in sprite table)
        ldx     plane_sprite_num
        stx     tmp2

        ; Change sprite data & mask
        lda     #<_plane_destroyed
        ldx     #>_plane_destroyed
        jsr     pushax
        lda     tmp2
        jsr     _replace_sprite

        ; Loop for seven images
        lda     #7
        sta     tmp1

        ; Disable interrupts
        php
        sei

:       lda     tmp2
        jsr     _load_sprite_pointer
        jsr     _setup_sprite_pointer
        jsr     _draw_sprite
        inc     plane_data+SPRITE_DATA::X_COORD
        lda     #32
        jsr     _platform_msleep
        dec     tmp1
        bne     :-

        plp

        ; Change sprite back
        lda     #<_plane
        ldx     #>_plane
        jsr     pushax
        lda     tmp2
        jsr     _replace_sprite

        rts
.endproc
