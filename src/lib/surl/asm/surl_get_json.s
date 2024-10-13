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

        .export   _surl_get_json

        ; Used by surl_find_line too
        .export   pop_tos_send_htons
        .export   get_len_ptr2
        .export   pop_tos_ret_empty_string
        .export   read_val_and_terminate

        .import   _serial_putc_direct
        .import   _simple_serial_puts
        .import   _simple_serial_puts_nl
        .import   _simple_serial_getc
        .import   _surl_read_with_barrier

        .import   pushax, pushptr1, popptr1, popax, popa
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

no_translit:      .asciiz "0"

pop_tos_send_htons:
        jsr       popax
send_htons:
        tay                   ; A->Y, X->A for htons
        txa
        jsr       _serial_putc_direct
        tya
        jmp       _serial_putc_direct

pop_tos_ret_empty_string:
        jsr       popptr1
        lda       #$00
        tay
        sta       (ptr1),y
        jmp       returnFFFF

read_val_and_terminate:
        ; (buffer is already at TOS, but save it
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
        rts

get_len_ptr2:
        lda       #<ptr2
        ldx       #>ptr2
        jsr       pushax
        lda       #$02
        ldx       #$00
        jmp       _surl_read_with_barrier

;static int __fastcall__ surl_get_json(char *buffer, const char *selector, const char *translit, char striphtml, size_t len) {
_surl_get_json:
        pha
        lda       #SURL_CMD_JSON
        jsr       _serial_putc_direct
        pla

        ; Send max_len (it's in AX not on stack)
        jsr       send_htons

        ; Send striphtml
        jsr       popa
        jsr       _serial_putc_direct

        ; Send translit
        jsr       popax
        cpx       #$00        ; Assume string isn't in ZP
        bne       :+
        lda       #<no_translit
        ldx       #>no_translit
:       jsr       _simple_serial_puts

        ; Send space separator
        lda       #$20
        jsr       _serial_putc_direct

        ; Send selector
        jsr       popax
        jsr       _simple_serial_puts_nl

        ; Get reply
        jsr       _simple_serial_getc
        cmp       #SURL_ERROR_OK
        beq       :+

        ; Return empty string
        jmp       pop_tos_ret_empty_string

:       ; Get result length
        jsr       get_len_ptr2

        ; And get value and return
        jsr       read_val_and_terminate

        ; Len is still in ptr2, which is still inverted
        ldx       ptr2
        lda       ptr2+1
        rts
