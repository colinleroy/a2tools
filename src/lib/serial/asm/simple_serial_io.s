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

        .export         _simple_serial_puts
        .export         _simple_serial_puts_nl
        .export         _simple_serial_write
        .export         _simple_serial_flush
        .export         simple_serial_compute_ptr_end

        .ifdef          SIMPLE_SERIAL_DUMP
        .export         _simple_serial_dump
        .import         popa
        .endif

        .import         _simple_serial_getc_with_timeout
        .import         _ser_get, _serial_putc_direct, _strlen
        .import         pushax, popax
        .importzp       tmp2, ptr3, ptr4
        .include        "apple2.inc"
        .include        "ser-error.inc"

        .ifdef SURL_TO_LANGCARD
        .segment "LC"
        .else
        .segment "LOWCODE"
        .endif

;void __fastcall__ simple_serial_puts(const char *buf) {
_simple_serial_puts:
        jsr     pushax
        jsr     _strlen
        jmp     _simple_serial_write

;void __fastcall__ simple_serial_puts_nl(const char *buf) {
_simple_serial_puts_nl:
        jsr       _simple_serial_puts
        lda       #$0A
        jmp       _serial_putc_direct

simple_serial_compute_ptr_end:
        sta     ptr3
        stx     ptr3+1
        jsr     popax
        sta     ptr4
        stx     ptr4+1
        clc
        adc     ptr3            ; set ptr3 to end
        sta     ptr3
        lda     ptr4+1
        adc     ptr3+1
        sta     ptr3+1
        rts

        .ifdef  SIMPLE_SERIAL_DUMP
_simple_serial_dump:
        jsr     simple_serial_compute_ptr_end
        lda     #$17            ; SURL_METHOD_DUMP
        jsr     _serial_putc_direct
        lda     #$0A            ; \n
        jsr     _serial_putc_direct

        jsr     popa            ; dump ID
        jsr     _serial_putc_direct

        lda     ptr4+1          ; Dump start
        jsr     _serial_putc_direct
        lda     ptr4
        jsr     _serial_putc_direct

        lda     ptr3+1          ; Dump end
        jsr     _serial_putc_direct
        lda     ptr3
        jsr     _serial_putc_direct

        jmp     write_again     ; Jump to serial_write without recomputing ptr_end
        .endif

; void __fastcall__ simple_serial_write(const char *ptr, size_t nmemb) {
_simple_serial_write:
        jsr     simple_serial_compute_ptr_end
write_again:
        ldy     #$00
        lda     (ptr4),y
        jsr     _serial_putc_direct

        inc     ptr4
        bne     :+
        inc     ptr4+1
:       lda     ptr4
        cmp     ptr3
        bne     write_again
        ldx     ptr4+1
        cpx     ptr3+1
        bne     write_again
        rts

_simple_serial_flush:
        jsr     _simple_serial_getc_with_timeout
        cpx     #$FF
        bne     _simple_serial_flush
        rts
