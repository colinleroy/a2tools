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


        .export   _hgr_hi, _hgr_low
        .export   _mod7_table, _div7_table

        .export   my_pusher_bgbackup, their_pusher_bgbackup, puck_bgbackup

        .include  "sprite.inc"
        .include  "my_pusher0.gen.inc"
        .include  "their_pusher4.gen.inc"
        .include  "puck0.gen.inc"

.bss

my_pusher_bgbackup:     .res my_pusher0_BYTES
their_pusher_bgbackup:  .res their_pusher4_BYTES
puck_bgbackup:          .res puck0_BYTES

_hgr_hi:                .res 192
_hgr_low:               .res 192

_div7_table:            .res 256
_mod7_table:            .res 256
