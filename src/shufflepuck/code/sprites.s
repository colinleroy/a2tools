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

                  .export my_pusher_data, their_pusher_data, puck_data, pointer_data, hand_data

                  .export my_pushers_low, my_pushers_high, my_pushers_height, my_pushers_bytes, my_pushers_bpline
                  .export their_pushers_low, their_pushers_high, their_pushers_height, their_pushers_bytes, their_pushers_bpline
                  .export pucks_low, pucks_high, pucks_height, pucks_bytes, pucks_bpline

                  .import _my_pusher0, _my_pusher1, _my_pusher2, _my_pusher3
                  .import _their_pusher4, _their_pusher5
                  .import _puck0, _puck1, _puck2, _puck3, _puck4, _puck5, _puck6
                  .import _pointer, _hand

                  .import  their_pusher_bgbackup, puck_bgbackup, hand_bgbackup

                  .include "my_pusher0.gen.inc"
                  .include "my_pusher1.gen.inc"
                  .include "my_pusher2.gen.inc"
                  .include "my_pusher3.gen.inc"
                  .include "their_pusher4.gen.inc"
                  .include "their_pusher5.gen.inc"
                  .include "puck0.gen.inc"
                  .include "puck1.gen.inc"
                  .include "puck2.gen.inc"
                  .include "puck3.gen.inc"
                  .include "puck4.gen.inc"
                  .include "puck5.gen.inc"
                  .include "puck6.gen.inc"
                  .include "pointer.gen.inc"
                  .include "hand.gen.inc"
                  .include "constants.inc"

.segment "LOWCODE"

; Our pusher
my_pusher_data:
                  .byte 0                   ; x
                  .byte my_pusher0_WIDTH
                  .byte 0                   ; y
                  .byte my_pusher0_HEIGHT
                  .byte 0                   ; prev_x
                  .byte 0                   ; prev_y
                  .byte my_pusher0_BYTES-1  ; bytes of sprite - 1
                  .byte my_pusher0_BPLINE-1 ; width of sprite in bytes
                  .addr _my_pusher0         ; sprites
                  .addr $0000               ; pointer to previous sprite to clear
                  .byte 0                   ; need clear

; variants
my_pushers_low:
        .byte <_my_pusher0
        .byte <_my_pusher1
        .byte <_my_pusher2
        .byte <_my_pusher3
my_pushers_high:
        .byte >_my_pusher0
        .byte >_my_pusher1
        .byte >_my_pusher2
        .byte >_my_pusher3
my_pushers_height:
        .byte my_pusher0_HEIGHT
        .byte my_pusher1_HEIGHT
        .byte my_pusher2_HEIGHT
        .byte my_pusher3_HEIGHT
my_pushers_bytes:
        .byte my_pusher0_BYTES-1
        .byte my_pusher1_BYTES-1
        .byte my_pusher2_BYTES-1
        .byte my_pusher3_BYTES-1
my_pushers_bpline:
        .byte my_pusher0_BPLINE-1
        .byte my_pusher1_BPLINE-1
        .byte my_pusher2_BPLINE-1
        .byte my_pusher3_BPLINE-1

; Their pusher
their_pusher_data:
                  .byte 0                     ; x
                  .byte their_pusher4_WIDTH
                  .byte 0                     ; y
                  .byte their_pusher4_HEIGHT
                  .byte 0                     ; prev_x
                  .byte 0                     ; prev_y
                  .byte their_pusher4_BYTES-1 ; bytes of sprite - 1
                  .byte their_pusher4_BPLINE-1; width of sprite in bytes
                  .addr _their_pusher4        ; sprites
                  .addr their_pusher_bgbackup ; background backup buffer
                  .byte 0                     ; need clear

; Variants
their_pushers_low:
        .byte <_their_pusher4
        .byte <_their_pusher5
their_pushers_high:
        .byte >_their_pusher4
        .byte >_their_pusher5
their_pushers_height:
        .byte their_pusher4_HEIGHT
        .byte their_pusher5_HEIGHT
their_pushers_bytes:
        .byte their_pusher4_BYTES-1
        .byte their_pusher5_BYTES-1
their_pushers_bpline:
        .byte their_pusher4_BPLINE-1
        .byte their_pusher5_BPLINE-1

; The puck
puck_data:
                  .byte 0               ; x
                  .byte puck0_WIDTH
                  .byte 0               ; y
                  .byte puck0_HEIGHT
                  .byte 0               ; prev_x
                  .byte 0               ; prev_y
                  .byte puck0_BYTES-1   ; bytes of sprite - 1
                  .byte puck0_BPLINE-1  ; width of sprite in bytes
                  .addr _puck0          ; sprites
                  .addr puck_bgbackup   ; background backup buffer
                  .byte 0               ; need clear

; Variants
pucks_low:
        .byte <_puck0
        .byte <_puck1
        .byte <_puck2
        .byte <_puck3
        .byte <_puck4
        .byte <_puck5
        .byte <_puck6
pucks_high:
        .byte >_puck0
        .byte >_puck1
        .byte >_puck2
        .byte >_puck3
        .byte >_puck4
        .byte >_puck5
        .byte >_puck6
pucks_height:
        .byte puck0_HEIGHT
        .byte puck1_HEIGHT
        .byte puck2_HEIGHT
        .byte puck3_HEIGHT
        .byte puck4_HEIGHT
        .byte puck5_HEIGHT
        .byte puck6_HEIGHT
pucks_bytes:
        .byte puck0_BYTES-1
        .byte puck1_BYTES-1
        .byte puck2_BYTES-1
        .byte puck3_BYTES-1
        .byte puck4_BYTES-1
        .byte puck5_BYTES-1
        .byte puck6_BYTES-1
pucks_bpline:
        .byte puck0_BPLINE-1
        .byte puck1_BPLINE-1
        .byte puck2_BPLINE-1
        .byte puck3_BPLINE-1
        .byte puck4_BPLINE-1
        .byte puck5_BPLINE-1
        .byte puck6_BPLINE-1

; Pointer
pointer_data:
                  .byte 0                   ; x
                  .byte pointer_WIDTH
                  .byte 0                   ; y
                  .byte pointer_HEIGHT
                  .byte 0                   ; prev_x
                  .byte 0                   ; prev_y
                  .byte pointer_BYTES-1     ; bytes of sprite - 1
                  .byte pointer_BPLINE-1    ; width of sprite in bytes
                  .addr _pointer            ; sprites
                  .assert puck0_BYTES > pointer_BYTES, error
                  .addr puck_bgbackup       ; Spare some memory there
                  .byte 0                   ; need clear


; Hand for scores
hand_data:
                  .byte 0                   ; x
                  .byte hand_WIDTH
                  .byte 0                   ; y
                  .byte hand_HEIGHT
                  .byte 0                   ; prev_x
                  .byte 0                   ; prev_y
                  .byte hand_BYTES-1        ; bytes of sprite - 1
                  .byte hand_BPLINE-1       ; width of sprite in bytes
                  .addr _hand               ; sprites
                  .addr hand_bgbackup
                  .byte 0                   ; need clear
