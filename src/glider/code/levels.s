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

                  .export cur_level
                  .export cur_level_logic

                  .export num_lives, num_rubber_bands, num_battery
                  .export cur_score, plane_sprite_num
                  .export plane_data, rubber_band_data

                  .import _plane, _plane_mask
                  .import _rubber_band, _rubber_band_mask

                  .import  plane_bgbackup
                  .import  rubber_band_bgbackup

                  .include "clock.gen.inc"
                  .include "plane.gen.inc"
                  .include "rubber_band.gen.inc"
                  .include "constants.inc"

.data

; There is always a single plane
plane_data:
                  .byte 1             ; active
                  .byte 0             ; deadly
                  .byte 1             ; destroyable
                  .byte 0             ; static
                  .byte plane_MIN_X   ; x
                  .byte plane_WIDTH
                  .byte plane_MIN_Y   ; y
                  .byte plane_HEIGHT
                  .byte plane_MAX_X   ; prev_x
                  .byte plane_MAX_Y   ; prev_y
                  .byte plane_BYTES-1 ; bytes of sprite - 1
                  .byte plane_BPLINE-1; width of sprite in bytes
                  .addr _plane        ; plane sprites
                  .addr _plane_mask   ; plane masks
                  .byte 0             ; deactivate data
                  .addr $0000         ; deactivate cb
                  .word $0000         ; state backup
                  .addr plane_bgbackup
                  .byte 0               ; need clear

; There is always a single rubber band
rubber_band_data:
                  .byte 0                   ; active
                  .byte 1                   ; deadly
                  .byte 0                   ; destroyable
                  .byte 0                   ; static
                  .byte rubber_band_MIN_X   ; x
                  .byte rubber_band_WIDTH
                  .byte rubber_band_MIN_Y   ; y
                  .byte rubber_band_HEIGHT
                  .byte rubber_band_MAX_X   ; prev_x
                  .byte rubber_band_MAX_Y   ; prev_y
                  .byte rubber_band_BYTES-1 ; bytes of sprite - 1
                  .byte rubber_band_BPLINE-1; width of sprite in bytes
                  .addr _rubber_band        ; band sprites
                  .addr _rubber_band_mask   ; band masks
                  .byte 0
                  .addr $0000
                  .word $0000
                  .addr rubber_band_bgbackup
                  .byte 0               ; need clear

cur_level:        .byte   0
num_lives:        .byte   NUM_LIVES
cur_score:        .word   0
num_rubber_bands: .byte   0
num_battery:      .byte   0

cur_level_logic:  .addr   $FFFF
plane_sprite_num: .byte   0
