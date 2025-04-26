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
        .export  _hgr_force_mono40, _text_mono40

        .import  _init_hgr, _init_text, hgr_mono_file

        .include "apple2.inc"
        .include "fcntl.inc"


HR1_OFF         := $C0B2
HR1_ON          := $C0B3
HR2_OFF         := $C0B4
HR2_ON          := $C0B5
HR3_OFF         := $C0B6
HR3_ON          := $C0B7
TEXT16_OFF      := $C0B8
TEXT16_ON       := $C0B9

.proc _hgr_force_mono40
        lda     #1
        jsr     _init_hgr
        lda     hgr_mono_file
        beq     :+
        bit       DHIRESON
:       rts
.endproc

.proc _text_mono40
        bit     DHIRESOFF
        jmp     _init_text
.endproc
