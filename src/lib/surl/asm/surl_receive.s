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

        .export   _surl_receive_bindata
        .export   _surl_strip_html
        .export   _surl_translit

        .import   _serial_putc_direct
        .import   _simple_serial_puts_nl
        .import   _surl_read_response_header
        .import   _surl_read_with_barrier
        .import   _resp

        .import   popax, pushax, push0ax
        .import   tossubeax, tosaddeax

        .importzp sreg, ptr1

        .include  "../../../surl-server/surl_protocol.inc"
        .include  "../../surl.inc"
        .include  "stdio.inc"

        .data

        .ifdef SURL_TO_LANGCARD
        .segment "LC"
        .else
        .segment "LOWCODE"
        .endif

;void __fastcall__ surl_strip_html(char strip_level) {
_surl_strip_html:
        ; Backup strip_level
        pha

        lda       #SURL_CMD_STRIPHTML
        jsr       _serial_putc_direct

        ; Send strip_level
        pla
        jsr       _serial_putc_direct

read_response_reset_pos:
        ; Read header
        jsr       _surl_read_response_header

        ; Reset cur_pos
        lda       #$00
        sta       _resp + SURL_RESPONSE::CUR_POS
        sta       _resp + SURL_RESPONSE::CUR_POS+1
        sta       _resp + SURL_RESPONSE::CUR_POS+2
        sta       _resp + SURL_RESPONSE::CUR_POS+3
        rts

;void __fastcall__ surl_strip_html(char *charset) {
_surl_translit:
        ; Backup charset low byte
        pha

        lda       #SURL_CMD_TRANSLIT
        jsr       _serial_putc_direct

        ; Restore charset low byte
        pla
        jsr       _simple_serial_puts_nl

        jmp       read_response_reset_pos

;size_t __fastcall__ surl_receive_bindata(char *buffer, size_t max_len, char binary) {
_surl_receive_bindata:
        ; Backup binary param
        pha

        ; Compute to_read: min(resp.size - resp.cur_pos, max_len)
        ; resp.size - resp.cur_pos
        ldx       _resp + SURL_RESPONSE::SIZE+3
        lda       _resp + SURL_RESPONSE::SIZE+2
        jsr       pushax
        ldx       _resp + SURL_RESPONSE::SIZE+1
        lda       _resp + SURL_RESPONSE::SIZE
        jsr       pushax

        lda       _resp + SURL_RESPONSE::CUR_POS+3
        sta       sreg+1
        lda       _resp + SURL_RESPONSE::CUR_POS+2
        sta       sreg
        ldx       _resp + SURL_RESPONSE::CUR_POS+1
        lda       _resp + SURL_RESPONSE::CUR_POS
        jsr       tossubeax

        ; Compute min
        ; Easy if resp.size-resp.cur_pos >= 0x00010000
        ldy       sreg+1
        bne       @min_is_param
        ldy       sreg
        bne       @min_is_param

        ; Otherwise compare last 16 bits - time to
        ; move the substraction's AX to sreg, we don't care
        ; about the two high bytes anymore
        stx       sreg+1
        sta       sreg

        ; pop max_len and re-push it, we'll have to pop
        ; it in min_is_param in case we come from the easy
        ; way.
        jsr       popax
        jsr       pushax
        cpx       sreg+1
        bcc       @min_is_param
        bne       @min_is_remainder

        ; high byte equal, compare low
        cmp       sreg
        bcc       @min_is_param

        ; if we're here either both are equal or min_is_remainder
@min_is_remainder:
        ; Pop max_len anyway and forget it
        jsr       popax
        ; remainder is already in sreg
        jmp       @check_len

@min_is_param:
        jsr       popax
        sta       sreg
        stx       sreg+1

@check_len:
        ; Check if to_read == 0 before sending command
        lda       sreg
        ldx       sreg+1
        bne       @send_command
        cmp       #$00
        bne       @send_command

        ; Nothing to read, pop buffer
        jsr       popax
        ; don't forget to pop binary param
        pla
        ; and return
        lda       #$00
        tax
        rts

@send_command:
        lda       #SURL_CMD_SEND
        jsr       _serial_putc_direct

@send_len:
        lda       sreg+1       ; htons'd
        tax
        jsr       _serial_putc_direct
        lda       sreg
        jsr       _serial_putc_direct

        ; binary mode?
        pla
        beq       @read_and_terminate
        ; Don't pop buffer, it will be popped by
        ; _surl_read_with_barrier
        ; Reload to_read
        lda       sreg
        jsr       _surl_read_with_barrier
        jmp       @inc_pos

@read_and_terminate:
        ; Pop and push buffer for termination
        jsr       popax
        jsr       pushax
        jsr       pushax
        ; Reload to_read
        lda       sreg
        ldx       sreg+1
        jsr      _surl_read_with_barrier

        ; Terminate
        jsr       popax
        clc
        adc       sreg
        sta       ptr1
        txa
        adc       sreg+1
        sta       ptr1+1
        lda       #$00
        tay
        sta       (ptr1),y

@inc_pos:
        lda       sreg
        ldx       sreg+1
        ; Backup for return
        jsr       pushax
        ; Push for addition
        jsr       push0ax

        lda       _resp + SURL_RESPONSE::CUR_POS+3
        sta       sreg+1
        lda       _resp + SURL_RESPONSE::CUR_POS+2
        sta       sreg
        ldx       _resp + SURL_RESPONSE::CUR_POS+1
        lda       _resp + SURL_RESPONSE::CUR_POS
        jsr       tosaddeax
        sta       _resp + SURL_RESPONSE::CUR_POS
        stx       _resp + SURL_RESPONSE::CUR_POS+1
        lda       sreg
        sta       _resp + SURL_RESPONSE::CUR_POS+2
        lda       sreg+1
        sta       _resp + SURL_RESPONSE::CUR_POS+3

        ; Return to_read
        jmp       popax
