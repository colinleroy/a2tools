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

        .export   _main

        .import   _init_hgr, _init_mouse
        .import   mouse_x, mouse_y
        .import   mouse_dx, mouse_dy

        .import   puck_x, puck_y, puck_dx, puck_dy
        .import   my_pusher_x, my_pusher_y

        .import   _puck_reinit_order
        .import   _draw_screen
        .import   _move_puck, _puck_check_hit
        .import   _move_my_pusher, _move_their_pusher

        .import   _load_table, _load_lowcode
        .import   hz

        .import   _mouse_wait_vbl
        .import   _mouse_reset
        .import   _mouse_calibrate_hz

        .import   _platform_msleep
        .import   _allow_lowercase
        .import   _build_hgr_tables

        .import   _init_text, _clrscr, _memcpy, pushax

        .include  "apple2.inc"
        .include  "sprite.inc"
        .include  "puck_coords.inc"
        .include  "my_pusher_coords.inc"
        .include  "my_pusher0.gen.inc"
        .include  "constants.inc"

.segment "LOWCODE"

.code

.proc _main
        jsr     _load_lowcode
        jmp     _real_main
.endproc

.segment "LOWCODE"

.proc _real_main
        jsr     _build_hgr_tables

        lda     #1
        jsr     _allow_lowercase  ; For Bulgarian i18n

        jsr     _init_mouse
        bcc     :+                ; Do we have a mouse?

        ; No. That won't work.
        brk

calibrate_hz_handler:
:       jsr     _mouse_calibrate_hz

.ifndef __APPLE2ENH__
        ; Give the Mousecard time to settle post-init
        lda     #$FF
        ldx     #0
        jsr     _platform_msleep
.endif

        jsr     _load_table

        lda     #<$4000 ; Backup table
        ldx     #>$4000
        jsr     pushax
        lda     #<$2000
        ldx     #>$2000
        jsr     pushax
        lda     #<$2100
        ldx     #>$2100
        jsr     _memcpy

        lda     #1
        jsr     _init_hgr

        jsr     _mouse_reset

        ; Wait for first interrupt
        jsr     _mouse_wait_vbl

reset_game:
        ; Initialize coords
        ldy     mouse_y
        sty     my_pusher_y
        ldx     mouse_x
        stx     my_pusher_x

        lda     #PUCK_INI_X
        sta     puck_x
        lda     #PUCK_INI_Y
        sta     puck_y

        lda     #0
        sta     puck_dx
        sta     puck_dy
        jsr     _puck_reinit_order
game_loop:
        ; the WAI of the poor
        ; because I don't understand how WAI works
        ; and I want to keep things 6502-ok
wait_vbl_handler:
        jsr     _mouse_wait_vbl

loop_start:
        jsr     _draw_screen

        ldy     mouse_y
        sty     my_pusher_y
        ldx     mouse_x
        stx     my_pusher_x
        jsr     _move_my_pusher
        ; 
        ; lda     mouse_y
        ; sec
        ; sbc     #90
        ; tay
        ; ldx     mouse_x
        ; jsr     _move_their_pusher

        jsr     _puck_check_hit
        jsr     _move_puck

        bcs     reset_game

        ; Next round!
        jmp     game_loop
.endproc
