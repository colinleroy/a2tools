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

        .export   _surl_find_line
        .export   _surl_find_header

        .import   _serial_putc_direct
        .import   _simple_serial_puts_nl
        .import   _simple_serial_getc
        .import   _surl_read_with_barrier

        .import   pushax, pushptr1, popptr1, popax
        .import   return0, returnFFFF

        .importzp ptr1, ptr2

        .include  "../../../surl-server/surl_protocol.inc"
        .include  "../../surl.inc"
        .include  "stdio.inc"

        .data

        .ifdef SURL_TO_LANGCARD
        .segment "LC"
        .else
        .segment "LOWCODE"
        .endif

;static int __fastcall__ surl_find_in(char *buffer, char *search_str, size_t max_len, MatchType matchtype) {
surl_find_in:
        ; Backup matchtype
        pha

        ; Send read mode
        lda       in_body
        beq       :+
        lda       #SURL_CMD_FIND
        bne       @send_cmd
:       lda       #SURL_CMD_FIND_HEADER
@send_cmd:
        jsr       _serial_putc_direct

        ; Send matchtype
        pla
        jsr       _serial_putc_direct

        ; Send max_len
        jsr       popax
        tay                   ; A->Y, X->A for htons
        txa
        jsr       _serial_putc_direct
        tya
        jsr       _serial_putc_direct

        ; Send search_str
        jsr       popax
        jsr       _simple_serial_puts_nl

        ; Get reply
        jsr       _simple_serial_getc
        cmp       #SURL_ERROR_OK
        beq       :+

        ; Get buffer
        jsr       popptr1
        lda       #$00
        tay
        sta       (ptr1),y
        jmp       returnFFFF

:       ; Get result length
        lda       #<ptr2
        ldx       #>ptr2
        jsr       pushax
        lda       #$02
        ldx       #$00
        jsr       _surl_read_with_barrier

        ; And get value (buffer is already at TOS, but save it
        ; for termination)
        jsr       popax
        jsr       pushax
        jsr       pushax

        ldx       ptr2        ; ntohs
        lda       ptr2+1
        jsr       _surl_read_with_barrier

        ; Terminate buffer
        jsr       popax
        clc
        adc       ptr2+1
        sta       ptr1
        txa
        adc       ptr2
        sta       ptr1+1
        lda       #$00
        tay
        sta       (ptr1),y

        ; And return
        tax
        rts

_surl_find_line:
        ldy       #1
        bne       :+
_surl_find_header:
        ldy       #0
:       sty       in_body
        jmp       surl_find_in

        .bss

in_body:          .res 1
