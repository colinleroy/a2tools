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
        .export   _surl_read_response_header
        .export   _surl_response_ok
        .export   _surl_response_code
        .export   _surl_content_type
        .export   _resp

        .import   _simple_serial_open
        .import   _serial_putc_direct
        .import   _simple_serial_set_irq
        .import   _simple_serial_flush
        .import   _simple_serial_close
        .import   _simple_serial_getc
        .import   _simple_serial_getc_with_timeout
        .import   _simple_serial_puts
        .import   _simple_serial_puts_nl

        .import   _malloc0
        .import   _surl_read_with_barrier

        .import   _free, _bzero
        .import   pusha, pushax, popa, popax
        .import   incsp6, tosudiva0
        .import   return0

        .importzp ptr1

        .include  "../../../surl-server/surl_protocol.inc"
        .include  "../../surl.inc"
        .include  "stdio.inc"

        .data

_surl_user_agent: .word $0000
proxy_opened:     .byte $00

        .rodata

ua_hdr:           .asciiz "User-Agent: "

        .ifdef SURL_TO_LANGCARD
        .segment "LC"
        .else
        .segment "LOWCODE"
        .endif

;char surl_connect_proxy(void)
.proc _surl_connect_proxy: near
        lda       proxy_opened
        bne       @connect_ok

        jsr       _simple_serial_open
        cmp       #$00
        beq       :+
        ; Error opening serial
        rts

:       lda       #SURL_METHOD_ABORT
        jsr       _serial_putc_direct

        lda       #1
        jsr       _simple_serial_set_irq

        ; Break previous session
        jsr       _simple_serial_flush
        lda       #SURL_METHOD_ABORT
        jsr       _serial_putc_direct
        lda       #$0A
        jsr       _serial_putc_direct
        jsr       _simple_serial_flush

@connect_ok:
        lda       #$01
        sta       proxy_opened
        jmp       return0
.endproc

;void surl_disconnect_proxy(void)
.proc _surl_disconnect_proxy: near
        lda       #$00
        sta       proxy_opened
        jmp       _simple_serial_close
.endproc

set_code_return_response:
        sta       _resp + SURL_RESPONSE::CODE
        stx       _resp + SURL_RESPONSE::CODE+1
load_response_ptr:
        lda       #<_resp
        ldx       #>_resp
        rts

free_content_type:
        ldx       _resp + SURL_RESPONSE::CONTENT_TYPE+1
        beq       :+
        lda       _resp + SURL_RESPONSE::CONTENT_TYPE
        jsr       _free
        lda       #$00
        sta       _resp + SURL_RESPONSE::CONTENT_TYPE
        sta       _resp + SURL_RESPONSE::CONTENT_TYPE+1
:       rts

;const surl_response * __fastcall__ surl_start_request(char **headers, unsigned char n_headers, char *url, const char method)
.proc _surl_start_request: near
        ; Save method
        jsr       pusha

        ; Free previous content_type
        jsr       free_content_type

        ; Clear struct
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

:       ; Send method
        jsr       popa
        sta       method
        jsr       _serial_putc_direct
        ; Send URL
        jsr       popax
        jsr       _simple_serial_puts_nl

        ; Send headers
        jsr       popa
        sta       i

        jsr       popax
        sta       hdrs
        stx       hdrs+1

        jmp       @check_next_hdr

@send_hdr:
        dec       i
        lda       i
        asl
        tay

        ; Load header
        lda       hdrs
        sta       ptr1
        lda       hdrs+1
        sta       ptr1+1
        iny
        lda       (ptr1),y
        tax
        dey
        lda       (ptr1),y
        jsr       _simple_serial_puts_nl

@check_next_hdr:
        lda       i
        bne       @send_hdr

@send_ua:
        ; Send User-Agent
        lda       _surl_user_agent+1
        beq       :+

        lda       #<ua_hdr
        ldx       #>ua_hdr
        jsr       _simple_serial_puts
        lda       _surl_user_agent
        ldx       _surl_user_agent+1
        jsr       _simple_serial_puts_nl

:       ; Send header/body separator
        lda       #$0A
        jsr       _serial_putc_direct

        ; Get answer
        jsr       _simple_serial_getc_with_timeout
        sta       i
        stx       i+1

        ; No response?
        cmp       #<EOF
        bne       :+
        cpx       #>EOF
        bne       :+
        lda       #<SURL_ERR_TIMEOUT
        ldx       #>SURL_ERR_TIMEOUT
        jmp       set_code_return_response

:       ldx       method

        ; SURL_METHOD_RAW => SURL_ANSWER_RAW_START 100 else protocol_error
@check_raw:
        cpx       #SURL_METHOD_RAW
        bne       @check_get_del
        cmp       #SURL_ANSWER_RAW_START
        bne       @ret_protocol_error
@ret_100:
        lda       #<SURL_HTTP_CONTINUE
        ldx       #>SURL_HTTP_CONTINUE
        jmp       set_code_return_response

@check_get_del:
        ; GET or DELETE, OK if we got WAIT, otherwise SURL_ERR_PROTOCOL_ERROR
        cpx       #SURL_METHOD_GET
        beq       @check_wait_or_protocol_error
        cpx       #SURL_METHOD_DELETE
        beq       @check_wait_or_protocol_error
        cpx       #SURL_METHOD_MKDIR
        bne       @check_posts
@check_wait_or_protocol_error:
        cmp       #SURL_ANSWER_WAIT
        bne       @ret_protocol_error
        jsr       _surl_read_response_header
        jmp       load_response_ptr

@ret_protocol_error:
        lda       #<SURL_ERR_PROTOCOL_ERROR
        ldx       #>SURL_ERR_PROTOCOL_ERROR
        jmp       set_code_return_response

@check_posts:
        cpx       #SURL_METHOD_POST_DATA
        beq       @check_send_size_fields_or_protocol_error
        cpx       #SURL_METHOD_POST
        beq       @check_send_size_fields_or_protocol_error
        cpx       #SURL_METHOD_PUT
        bne       @check_ping
@check_send_size_fields_or_protocol_error:
        cmp       #SURL_ANSWER_SEND_SIZE
        beq       @ret_100
        cmp       #SURL_ANSWER_SEND_SIZE
        beq       @ret_100
        bne       @ret_protocol_error

        ; PING, OK if we got PONG, get protocol version into code
@check_ping:
        cpx       #SURL_METHOD_PING
        bne       @check_streams
        cmp       #SURL_ANSWER_PONG
        bne       @ret_protocol_error
        lda       #SURL_CLIENT_READY
        jsr       _serial_putc_direct
        jsr       _simple_serial_getc
        ; A now contains protocol version
        ldx       #$00
        jmp       set_code_return_response

        ; STREAM_*, we either have 100 wait or must read response
        ; headers
@check_streams:
        cpx       #SURL_METHOD_STREAM_VIDEO
        beq       @check_wait_and_ret_100
        cpx       #SURL_METHOD_STREAM_AUDIO
        beq       @check_wait_and_ret_100
        cpx       #SURL_METHOD_STREAM_AV
        bne       @check_gettime
@check_wait_and_ret_100:
        cmp       #SURL_ANSWER_WAIT
        beq       @ret_100
        bne       @ret_protocol_error

        ; GETTIME
@check_gettime:
        cpx       #SURL_METHOD_GETTIME
        bne       @ret_protocol_error
        cmp       #SURL_ANSWER_TIME
        bne       @ret_protocol_error
        lda       #<SURL_HTTP_OK
        ldx       #>SURL_HTTP_OK
        jmp       set_code_return_response

        brk       ; We shouldn't be there
.endproc

.proc _surl_read_response_header: near
        jsr       free_content_type

        jsr       load_response_ptr
        jsr       pushax
        lda       #<SURL_RESPONSE_HDR_SIZE
        ldx       #>SURL_RESPONSE_HDR_SIZE
        jsr       _surl_read_with_barrier

        ; ntohl size
        lda       _resp + SURL_RESPONSE::SIZE+3
        ldx       _resp + SURL_RESPONSE::SIZE
        stx       _resp + SURL_RESPONSE::SIZE+3
        sta       _resp + SURL_RESPONSE::SIZE
        lda       _resp + SURL_RESPONSE::SIZE+2
        ldx       _resp + SURL_RESPONSE::SIZE+1
        stx       _resp + SURL_RESPONSE::SIZE+2
        sta       _resp + SURL_RESPONSE::SIZE+1

        ; ntohs code
        lda       _resp + SURL_RESPONSE::CODE+1
        ldx       _resp + SURL_RESPONSE::CODE
        stx       _resp + SURL_RESPONSE::CODE+1
        sta       _resp + SURL_RESPONSE::CODE

        ; ntohs header_size
        lda       _resp + SURL_RESPONSE::HEADER_SIZE+1
        ldx       _resp + SURL_RESPONSE::HEADER_SIZE
        stx       _resp + SURL_RESPONSE::HEADER_SIZE+1
        sta       _resp + SURL_RESPONSE::HEADER_SIZE

        ; ntohs content_type_size
        lda       _resp + SURL_RESPONSE::CONTENT_TYPE_SIZE+1
        ldx       _resp + SURL_RESPONSE::CONTENT_TYPE_SIZE
        stx       _resp + SURL_RESPONSE::CONTENT_TYPE_SIZE+1
        sta       _resp + SURL_RESPONSE::CONTENT_TYPE_SIZE

        ; Allocate content-type
        jsr       _malloc0
        sta       _resp + SURL_RESPONSE::CONTENT_TYPE
        stx       _resp + SURL_RESPONSE::CONTENT_TYPE+1

        ; Read content-type
        jsr       pushax
        lda       _resp + SURL_RESPONSE::CONTENT_TYPE_SIZE
        ldx       _resp + SURL_RESPONSE::CONTENT_TYPE_SIZE+1
        jmp       _surl_read_with_barrier
.endproc

.proc _surl_response_ok: near
        jsr       _surl_response_code
        jsr       pushax
        lda       #<100
        ldx       #>100
        jsr       tosudiva0
        cpx       #$00
        bne       @resp_nok
        cmp       #$02
        bne       @resp_nok
        lda       #1          ; X is 0 there
        rts
@resp_nok:
        lda       #0
        tax
        rts
.endproc

.proc _surl_response_code: near
        lda       _resp + SURL_RESPONSE::CODE
        ldx       _resp + SURL_RESPONSE::CODE+1
        rts
.endproc

.proc _surl_content_type: near
        lda       _resp + SURL_RESPONSE::CONTENT_TYPE
        ldx       _resp + SURL_RESPONSE::CONTENT_TYPE+1
        rts
.endproc

        .bss

_resp:            .tag SURL_RESPONSE

method:           .res 1
i:                .res 2
hdrs:             .res 2
