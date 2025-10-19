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

        .export  plane_bgbackup
        .export  rubber_band_bgbackup
        .export  _hgr_hi, _hgr_low, _hgr_bit
        .export  _mod7_table, _div7_table

        .export  sprites_bgbackup

        .include "sprite.inc"
        .include  "plane.gen.inc"
        .include  "rubber_band.gen.inc"

.bss

.align $100
sprites_bgbackup:      .res MAX_SPRITES*128

.align $100
_hgr_hi:               .res 192
plane_bgbackup:        .res plane_BYTES
rubber_band_bgbackup:  .res rubber_band_BYTES

.align $100
_div7_table:            .res 256
_mod7_table:            .res 256

.align $100
_hgr_low:               .res 192
_hgr_bit:               .res 7
