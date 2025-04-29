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

        .export   _home
        .export   _cout
        .export   _strout
        .export   _numout
        .export   _read_key
        .export   _num_to_buf
        .import   _utoa
        .import   pushax
        .import   ntoabuf

        .importzp ptr1

        .include  "apple2.inc"

.proc _home
        bit     $C082
        jsr     $FC58
        ; Fallthrough to bank_lc_out
.endproc
.proc bank_lc_out
        bit     $C080
        rts
.endproc

.proc _cout
        bit     $C082
        ora     #$80
        jsr     $FDED
        jmp     bank_lc_out
.endproc

.proc _strout
        sta     ptr1
        stx     ptr1+1

        ldy     #$00
:       lda     (ptr1),y
        beq     out
        jsr     _cout
        iny
        bne     :-
out:    rts
.endproc

.proc _num_to_buf
        jsr     pushax        ; Push number
        lda     #<ntoabuf
        ldx     #>ntoabuf

        jsr     pushax
        lda     #10
        ldx     #0

        jsr     _utoa

        lda     #<ntoabuf
        ldx     #>ntoabuf
        rts
.endproc

.proc _numout
        jsr     _num_to_buf
        jmp     _strout
.endproc

.proc _read_key
        bit     KBDSTRB
:       lda     KBD
        bpl     :-
        bit     KBDSTRB
        and     #$7F
        rts
.endproc
