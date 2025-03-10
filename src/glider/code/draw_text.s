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

        .export    _print_number
        .export    _read_string
        .export    _str_input

        .import   do_bin2dec_16bit, bcd_input, bcd_result

        .import    _font
        .import    _hgr_hi, _hgr_low

        .import    _cputc
        .importzp  tmp1, tmp2

        .include   "apple2.inc"
        .include   "font.gen.inc"

.segment "LOWCODE"

; A: ASCII code of the char
; X: X coord / 7
; Y: bottom Y coord
.proc _print_char
        stx     char_x_offset+1
        asl
        tax
        lda     _font,x
        sta     char_data+1
        lda     _font+1,x
        sta     char_data+2

        ldx     #font_HEIGHT-1
        clc

:       lda     _hgr_low,y
char_x_offset:
        adc     #$FF
        sta     scr+1
        lda     _hgr_hi,y
        adc     #0
        sta     scr+2

char_data:
        lda     $FFFF,x
scr:
        sta     $FFFF
        dey
        dex
        bpl     :-
        rts
.endproc

; A: low byte of number, high byte in bcd_input+1
; X: X coordinate
; Y: Y coordinate (bottom)
.proc _print_number
        sta     bcd_input
        stx     dest_x+1
        sty     blit_digit+1
        jsr     do_bin2dec_16bit

        lda     #$00
        sta     tmp1              ; To check whether we printed a number
        ldx     #$00
        stx     cur_char+1
char:
        lda     bcd_result,x
        bne     blit_digit        ; It's not 0 print it

        ; Is it a leading zero?
        ldy     tmp1
        bne     blit_digit        ; No

        ; If so, skip it without advancing cursor
        inc     cur_char+1
        bne     cur_char

blit_digit:
        ldy     #$FF
dest_x:
        ldx     #$FF
        cpx     #40               ; Don't blit out of screen
        bcs     out

        jsr     _print_char
        inc     tmp1              ; Note we printed a number
        inc     dest_x+1
        inc     cur_char+1
cur_char:
        ldx     #$FF
        cpx     #8                ; .sizeof(bcd_result)
        bcc     char

        ; Did we print anything? If not it's a 0
        lda     tmp1
        beq     blit_digit
out:
        rts
.endproc

; X: start X coordinate
; Y: bottom Y coordinate
; A: Maximum string length
.proc _read_string
        sta     max_str_len+1     ; And max string length
        ldx     #$00
        stx     tmp2              ; String len

        lda     #$00
        sta     _str_input,x      ; Terminate the string

        bit     KBDSTRB
get_key:
        lda     #'_'
        jsr     _cputc
        dec     CH

        lda     KBD
        bpl     get_key
        bit     KBDSTRB

        and     #$7F
        cmp     #$0D              ; Is it enter?
        beq     validate
        cmp     #$08              ; Is it left arrow?
        beq     delete_char
        cmp     #$7F              ; Or backspace?
        beq     delete_char
        cmp     #' '              ; We only take space to z
        bcc     get_key

        cmp     #'z'+1              ; Check if lower-case
        bcs     get_key

        ; We can print that letter
        ldx     tmp2
max_str_len:
        cpx     #$FF               ; Only N chars max, otherwise wait for Enter
        bcs     get_key

        ldx     tmp2
        sta     _str_input,x      ; Save char to string

        inc     tmp2

        jsr     _cputc

terminate:
        ldx     tmp2
        lda     #$00
        sta     _str_input,x      ; Terminate the string
        jmp     get_key

delete_char:
        ldx     tmp2
        beq     get_key

        dec     tmp2

        ; Remove underscore
        lda     #' '
        jsr     _cputc
        dec     CH
        dec     CH
        jmp     terminate

validate:
        rts
.endproc

        .bss

_str_input: .res 32
