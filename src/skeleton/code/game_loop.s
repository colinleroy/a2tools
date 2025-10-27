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

        .export     _draw_screen, _clear_screen
        .export     _move_sprite

        .import     _draw_sprite, _clear_sprite
        .import     mouse_x, mouse_y
        .import     _read_mouse

        .import     _play_puck_hit
        .import     _play_puck
        .import     _play_crash

        .import     puck_data

        .importzp   tmp1, ptr1

        .include    "sprite.inc"
        .include    "constants.inc"

my_pusher_y = mouse_y
my_pusher_x = mouse_x

.segment "LOWCODE"

.proc _clear_screen
        jmp     clear_puck
.endproc

.proc clear_puck
        lda     #<puck_data
        ldx     #>puck_data
        jmp     _clear_sprite
.endproc

.proc draw_puck
        lda     #<puck_data
        ldx     #>puck_data
        jmp     _draw_sprite
.endproc

.proc _draw_screen
        jsr     clear_puck
        jmp     draw_puck
.endproc

.proc _move_sprite
        ; Directly translate mouse coordinates to pusher coordinates
        jsr     _read_mouse

        lda     mouse_x
        sta     puck_data+SPRITE_DATA::X_COORD
        lda     mouse_y
        sta     puck_data+SPRITE_DATA::Y_COORD
        rts
.endproc

.bss
tmpx:            .res 1
tmpy:            .res 1

puck_right_x:    .res 1

puck_x:          .res 1
puck_precise_x:  .res 2
puck_dx:         .res 2

puck_y:          .res 1
puck_precise_y:  .res 2
puck_dy:         .res 2

their_pusher_x:  .res 1
their_pusher_y:  .res 1
their_pusher_dx: .res 1
their_pusher_dy: .res 1

my_pusher_real_gy:    .res 1
their_pusher_real_gy: .res 1
puck_real_gy:         .res 1
my_pusher_h:          .res 1
their_pusher_h:       .res 1
puck_h:               .res 1

puck_backup: .res 10

player_caught:              .res 1
puck_in_front_of_me:        .res 1
puck_in_front_of_them:      .res 1
player_puck_delta_change:   .res 1

prev_puck_in_front_of_me:   .res 1
prev_puck_in_front_of_them: .res 1

my_pusher_mid_x:            .res 1
my_currently_hitting:       .res 1
their_currently_hitting:    .res 1
bounces:                    .res 1

; For serial
their_hit_check_via_serial: .res 1
