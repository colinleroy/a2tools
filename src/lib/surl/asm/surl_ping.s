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

        .export   _surl_ping

        .import   _surl_start_request
        .import   _surl_response_code
        .import   _surl_disconnect_proxy
        .import   _simple_serial_configure
        .import   _simple_serial_flush
        .import   _clrscr
        .import   _cgetc
        .import   _tolower
        .import   _cputs
        .import   _citoa
        .import   pusha, pushax

        .include  "../../../surl-server/surl_protocol.inc"
        .include  "../../surl.inc"

        .rodata

estab_str:           .asciiz "Establishing serial connection... "
ping_method:         .asciiz "ping://"
crlfcrlf:            .byte $0D,$0A,$0D,$0A,$00
timeout_str:         .asciiz "Timeout"
cant_open_str:       .asciiz "Can not open serial port"
proto_mismatch1_str: .asciiz "surl-server Protocol "
proto_mismatch2_str: .asciiz " required, got "
press_key_str:       .byte   ". Press any key to try again or C to configure.",$0D,$0A,$00

        .segment "RT_ONCE"

;void __fastcall__ surl_ping(void)
.proc _surl_ping: near
@try_again:
        jsr       _clrscr
        lda       #<estab_str
        ldx       #>estab_str
        jsr       _cputs

        lda       #$00
        tax
        jsr       pushax              ; NULL,
        jsr       pusha               ; 0,
        lda       #<ping_method       ; "ping://",
        ldx       #>ping_method
        jsr       pushax
        lda       #SURL_METHOD_PING   ; SURL_METHOD_PING
        jsr       _surl_start_request
        jsr       _surl_response_code
        sta       response_code
        stx       response_code+1

        cmp       #<SURL_PROTOCOL_VERSION
        bne       @wrong_version
        cpx       #>SURL_PROTOCOL_VERSION
        bne       @wrong_version

@done:
        jmp       _clrscr

@wrong_version:
        cmp       #<SURL_ERR_PROTOCOL_ERROR
        bne       @check_error
        cpx       #>SURL_ERR_PROTOCOL_ERROR
        bne       @check_error

        ; Flush and try again
        jsr       _simple_serial_flush
        jmp       @try_again

@check_error:
        cmp       #<SURL_ERR_TIMEOUT
        bne       @check_port
        cpx       #>SURL_ERR_TIMEOUT
        bne       @check_port

        lda       #<timeout_str
        ldx       #>timeout_str
        jsr       _cputs
        jmp       @get_key

@check_port:
        cmp       #<SURL_ERR_PROXY_NOT_CONNECTED
        bne       @check_proto
        cpx       #>SURL_ERR_PROXY_NOT_CONNECTED
        bne       @check_proto

        lda       #<cant_open_str
        ldx       #>cant_open_str
        jsr       _cputs
        jmp       @get_key

@check_proto:
        lda       #<proto_mismatch1_str
        ldx       #>proto_mismatch1_str
        jsr       _cputs
        lda       #<SURL_PROTOCOL_VERSION
        ldx       #>SURL_PROTOCOL_VERSION
        jsr       _citoa
        lda       #<proto_mismatch2_str
        ldx       #>proto_mismatch2_str
        jsr       _cputs
        lda       response_code
        ldx       response_code+1
        jsr       _citoa

@get_key:
        lda       #<press_key_str
        ldx       #>press_key_str
        jsr       _cputs

        jsr       _cgetc
        jsr       _tolower
        cmp       #'c'
        bne       :+

        jsr       _surl_disconnect_proxy
        jsr       _simple_serial_configure
:       jmp       @try_again
.endproc

        .bss
response_code: .res 2
