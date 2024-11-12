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

        .export         _simple_serial_read

        .import         simple_serial_compute_ptr_end
        .import         throbber_on, throbber_off
        .import         _ser_get

        .importzp       ptr3, ptr4
        .include        "apple2.inc"
        .include        "ser-error.inc"

        .ifdef SURL_TO_LANGCARD
        .segment "LC"
        .else
        .segment "LOWCODE"
        .endif

;void __fastcall__ simple_serial_read(char *ptr, size_t nmemb) {
.proc _simple_serial_read: near
        jsr     simple_serial_compute_ptr_end
        jsr     throbber_on

        lda     #$00
        beq     check_bound
read_again:
        lda     ptr4
read_again_aok:
        ldx     ptr4+1
read_again_axok:
        jsr     _ser_get
        cmp     #SER_ERR_NO_DATA
        beq     read_again

        inc     ptr4
        bne     check_bound
        inc     ptr4+1

check_bound:
        lda     ptr4
        cmp     ptr3
        bne     read_again_aok
        ldx     ptr4+1
        cpx     ptr3+1
        bne     read_again_axok

        jmp     throbber_off
.endproc
