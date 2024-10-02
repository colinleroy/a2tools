;
; Copyright (C) 2022-2024 Colin Leroy-Mira <colin@colino.net>
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
;

        .export         _malloc0

        .import         _malloc, _bzero, swapstk, pushax

_malloc0:
        cmp       #0          ; Return NULL if size == 0
        bne       :+
        cpx       #0
        bne       :+
        rts

:       jsr       pushax      ; Backup size
        jsr       _malloc     ; Allocate

        cmp       #0          ; Die if failed
        bne       :+
        cpx       #0
        bne       :+
        brk

:       jsr       swapstk     ; push pointer and get size back
        jmp       _bzero      ; and return zeroed buffer
