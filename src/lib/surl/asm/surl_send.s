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

        .export   _surl_send_data_params
        .export   _surl_send_data_chunk

        .import   _serial_putc_direct
        .import   _simple_serial_getc
        .import   _simple_serial_write

        .import   popax, pushax, incsp6
        .import   return0, returnFFFF

        .importzp sp

        .include  "../../../surl-server/surl_protocol.inc"
        .include  "../../surl.inc"
        .include  "stdio.inc"

        .data

        .ifdef SURL_TO_LANGCARD
        .segment "LC"
        .else
        .segment "LOWCODE"
        .endif

;int __fastcall__ surl_send_data_params(uint32 total, int mode)
.proc _surl_send_data_params: near
        ; As we want to write total, then mode in big endian mode,
        ; just push mode on the stack,
        jsr       pushax
        ; then walk the stack in reverse order
        ldy       #5
:       lda       (sp),y
        jsr       _serial_putc_direct
        dey
        bpl       :-

        ; Drop stack
        jsr       incsp6

        ; And get go
        jsr       _simple_serial_getc
        cmp       #SURL_UPLOAD_GO
        bne       :+
        jmp       return0
:       jmp       returnFFFF
.endproc

.proc _surl_send_data_chunk: near
        tay
        lda       #SURL_CLIENT_READY
        jsr       _serial_putc_direct

        ; Send chunk_size in network order
        txa
        jsr       _serial_putc_direct
        tya
        jsr       _serial_putc_direct

        jmp       _simple_serial_write
.endproc
