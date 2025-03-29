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

                  .export my_pusher_data, their_pusher_data, puck_data

                  .import _my_pusher0, _their_pusher4, _puck0

                  .import  my_pusher_bgbackup, their_pusher_bgbackup, puck_bgbackup

                  .include "my_pusher0.gen.inc"
                  .include "their_pusher4.gen.inc"
                  .include "puck0.gen.inc"
                  .include "constants.inc"

.data

; Our pusher
my_pusher_data:
                  .byte (MY_PUSHER_INI_X/2)   ; x
                  .byte my_pusher0_WIDTH
                  .byte (MY_PUSHER_INI_Y/2)   ; y
                  .byte my_pusher0_HEIGHT
                  .byte (MY_PUSHER_INI_X/2) ; prev_x
                  .byte (MY_PUSHER_INI_Y/2) ; prev_y
                  .byte my_pusher0_BYTES-1 ; bytes of sprite - 1
                  .byte my_pusher0_BPLINE-1; width of sprite in bytes
                  .addr _my_pusher0        ; sprites
                  .addr my_pusher_bgbackup
                  .byte 0               ; need clear

; Their pusher
their_pusher_data:
                  .byte THEIR_PUSHER_INI_X   ; x
                  .byte their_pusher4_WIDTH
                  .byte THEIR_PUSHER_INI_Y   ; y
                  .byte their_pusher4_HEIGHT
                  .byte THEIR_PUSHER_INI_X ; prev_x
                  .byte THEIR_PUSHER_INI_Y ; prev_y
                  .byte their_pusher4_BYTES-1 ; bytes of sprite - 1
                  .byte their_pusher4_BPLINE-1; width of sprite in bytes
                  .addr _their_pusher4        ; sprites
                  .addr their_pusher_bgbackup
                  .byte 0               ; need clear

; The puck
puck_data:
                  .byte PUCK_INI_X   ; x
                  .byte puck0_WIDTH
                  .byte PUCK_INI_Y   ; y
                  .byte puck0_HEIGHT
                  .byte PUCK_INI_X ; prev_x
                  .byte PUCK_INI_Y ; prev_y
                  .byte puck0_BYTES-1 ; bytes of sprite - 1
                  .byte puck0_BPLINE-1; width of sprite in bytes
                  .addr _puck0        ; sprites
                  .addr puck_bgbackup
                  .byte 0               ; need clear
