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

        .export   _plot_line_applesoft

        .import   dx, dy, ox, oy

.proc _plot_line_applesoft
        bit     $C082
        ldx     #3            ; WHITE
        jsr     $F6EC         ; SETHCOL

        ldx     ox
        lda     oy
        ldy     #0
        jsr     $F457         ; HPOSN

        lda     dx
        ldy     dy
        ldx     #0
        jsr     $F53A         ; HLIN
        bit     $C080
        rts
.endproc
