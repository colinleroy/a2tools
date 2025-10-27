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

                  .export puck_data
                  .import _puck0
                  .import  puck_bgbackup

                  .include "puck0.gen.inc"
                  .include "constants.inc"

.segment "LOWCODE"

; The puck
.proc puck_data
                  .byte 0               ; x
                  .byte puck0_WIDTH
                  .byte 0               ; y
                  .byte puck0_HEIGHT
                  .byte 0               ; prev_x
                  .byte 0               ; prev_y
                  .byte puck0_BYTES     ; bytes of sprite
                  .byte puck0_BPLINE-1  ; width of sprite in bytes
                  .addr _puck0          ; sprites
                  .addr puck_bgbackup   ; background backup buffer
                  .assert <puck_bgbackup = $00, error ; Make sure it's aligned
                  .byte 0               ; need clear
.endproc
