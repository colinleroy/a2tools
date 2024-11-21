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

        .export         _realloc_safe

        .import         _realloc, popax, return0

_realloc_safe:
        cmp       #0          ; size 0 ?
        bne       :+
        cpx       #0
        bne       :+

        jsr       popax       ; Return NULL if size == 0
        jmp       return0

:       jsr       _realloc    ; Realloc
        cpx       #0
        bne       :+          ; Check it worked

        brk

:       rts
