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

        .export   _surl_user_agent
        .export   _surl_connect_proxy
        .export   _surl_disconnect_proxy
        .export   _surl_start_request
        .export   _resp

        .import   _simple_serial_open
        .import   _simple_serial_setup_no_irq_regs
        .import   _serial_putc_direct
        .import   _simple_serial_set_irq
        .import   _simple_serial_flush
        .import   _simple_serial_close

        .import   _free, _bzero
        .import   pusha, pushax
        .import   incsp6
        .import   return0

        .include  "../../../surl-server/surl_protocol.inc"

        .data

_surl_user_agent: .word $0000
proxy_opened:     .byte $00

        .rodata

        .ifdef SURL_TO_LANGCARD
        .segment "LC"
        .else
        .segment "LOWCODE"
        .endif

;char surl_connect_proxy(void)
_surl_connect_proxy:
        lda       proxy_opened
        bne       @connect_ok

        jsr       _simple_serial_open
        cmp       #$00
        beq       :+
        ; Error opening serial
        rts

:       jsr       _simple_serial_setup_no_irq_regs
        lda       #SURL_METHOD_ABORT
        jsr       _serial_putc_direct

        lda       #1
        jsr       _simple_serial_set_irq

        ; Break previous session
        jsr       _simple_serial_flush
        lda       #SURL_METHOD_ABORT
        jsr       _serial_putc_direct
        lda       #'\n'
        jsr       _serial_putc_direct
        jsr       _simple_serial_flush

@connect_ok:
        lda       #$01
        sta       proxy_opened
        jmp       return0

;void surl_disconnect_proxy(void)
_surl_disconnect_proxy:
        lda       #$00
        sta       proxy_opened
        jmp       _simple_serial_close

set_code_return_response:
        sta       _resp + SURL_RESPONSE::CODE
        stx       _resp + SURL_RESPONSE::CODE+1
load_response_ptr:
        lda       #<_resp
        ldx       #>_resp
        rts

;const surl_response * __fastcall__ surl_start_request(const char method, char *url, char **headers, unsigned char n_headers)
_surl_start_request:
        ; Save n_headers
        jsr       pusha

        ; Free previous content_type
        ldx       _resp + SURL_RESPONSE::CONTENT_TYPE+1
        beq       :+
        lda       _resp + SURL_RESPONSE::CONTENT_TYPE
        jsr       _free
        lda       #$00
        sta       _resp + SURL_RESPONSE::CONTENT_TYPE
        sta       _resp + SURL_RESPONSE::CONTENT_TYPE+1

:       ; Clear struct
        jsr       load_response_ptr
        jsr       pushax
        lda       #<.sizeof(SURL_RESPONSE)
        ldx       #>.sizeof(SURL_RESPONSE)
        jsr       _bzero

        ; Connect proxy if needed
        lda       proxy_opened
        bne       :+
        jsr       _surl_connect_proxy
        beq       :+

        jsr       incsp6      ; Pop all parameters
        lda       #<SURL_ERR_PROXY_NOT_CONNECTED
        ldx       #>SURL_ERR_PROXY_NOT_CONNECTED
        jmp       set_code_return_response

:       ; Send method and URL
        jsr       popa
        jsr       _simple_serial_putc
        jsr       popax
        jsr       _simple_serial_puts
        lda       #'\n'
        jsr       _simple_serial_putc

        .bss

_resp:  .tag SURL_RESPONSE
