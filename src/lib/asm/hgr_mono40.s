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
        .export  _hgr_force_mono40, _text_mono40, _hgr_unset_mono40
        .export  hgr_mono_file
        .import  _init_hgr, _init_text

        .include "apple2.inc"
        .include "fcntl.inc"

.proc _hgr_force_mono40
        lda     #1
        jsr     _init_hgr
        lda     hgr_mono_file
        beq     :+
        bit     DHIRESON
:       rts
.endproc

.proc _hgr_unset_mono40
        bit     DHIRESOFF
        rts
.endproc

.proc _text_mono40
        jsr     _hgr_unset_mono40
        jmp     _init_text
.endproc

        .bss

hgr_mono_file: .res 1
