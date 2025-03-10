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
        .export  _hgr_hi, _hgr_low
        .export  _mod7_table, _div7_table

        .export  sprites_bgbackup, large_tmp_buf

        .include "sprite.inc"
        .include  "plane.gen.inc"
        .include  "rubber_band.gen.inc"

.bss

; This is a big buffer that needs to hold compressed level data while
; uncompressing it (in load_data.s)
large_tmp_buf:        .res MAX_COMPRESSED_DATA_SIZE

; However, it is useless between level loads, so we will reuse it to
; store background saves for every sprite.
plane_bgbackup       = large_tmp_buf        + 0
rubber_band_bgbackup = plane_bgbackup       + plane_BYTES
sprites_bgbackup     = rubber_band_bgbackup + rubber_band_BYTES

; We also use the same buffer to store HGR and /7 tables
; This means HGR and division tables must be rebuilt after
; each time the buffer is used for decompression. See
; load_data.s.
_hgr_hi              = sprites_bgbackup     + MAX_SPRITES*128
_hgr_low             = _hgr_hi              + 192

_div7_table          = _hgr_low             + 192
_mod7_table          = _div7_table          + 256

__end_of_buffer_share= _mod7_table          + 256

; Make sure we don't overflow our large buffer
.assert __end_of_buffer_share < large_tmp_buf+MAX_COMPRESSED_DATA_SIZE, error
