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

        .export   _plot_dot

        .import   _hgr_hi, _hgr_low, _hgr_bit
        .import   _mod7_table, _div7_table
        .import   ox, oy, dx, dy

        .importzp ptr1

        .include  "helpers.inc"

; X,Y: x-y coordinates
.proc _plot_dot
        lda     _hgr_low,y
        clc
        adc     _div7_table,x
        sta     ptr1
        lda     _hgr_hi,y
        sta     ptr1+1
        ldy     #$00
        lda     _mod7_table,x
        tax
        lda     _hgr_bit,x
        ora     (ptr1),y
        sta     (ptr1),y
        rts
.endproc
