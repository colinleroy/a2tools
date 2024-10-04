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

        .export         _simple_serial_getc_immediate

        .import         _ser_get

        .importzp       tmp2

        .include        "apple2.inc"
        .include        "ser-error.inc"

        .ifdef SURL_TO_LANGCARD
        .segment "LC"
        .else
        .segment "LOWCODE"
        .endif

;int __fastcall__ simple_serial_getc_immediate(void) {
.proc _simple_serial_getc_immediate: near
        lda     #<tmp2
        ldx     #>tmp2
        jsr     _ser_get
        cmp     #SER_ERR_NO_DATA
        beq     no_char
        lda     tmp2
        ldx     #$00
        rts
no_char:
        lda     #$FF
        tax
        rts
.endproc
