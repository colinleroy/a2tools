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

        .export     _freq
        .import     _get_tv, _home, _strout, _read_key

        .importzp   ptr1

        .constructor calibrate_hz, 1

        .include    "apple2.inc"
        .include    "freq.inc"

.segment "ONCE"

.proc calibrate_hz
        jsr     _get_tv
        cmp     #TV_OTHER
        beq     :+
        sta     _freq
        rts

        ; We don't know. Let's ask.
:       jsr     _home
        lda     #<iip_str
        ldx     #>iip_str
        jsr     _strout

:       jsr     _read_key
        sec
        sbc     #'1'          ; '1'-'1'=0, TV_NTSC
        cmp     #2            ; 0/1 accepted
        bcs     :-

        sta     _freq
        bit     $C080
        rts
.endproc

iip_str:        .byte "HIT 1 FOR US (60HZ), 2 FOR EURO (50HZ)", $0D, $00

.segment "DATA"
; The only thing remaining from that code after init
_freq:          .res 1
