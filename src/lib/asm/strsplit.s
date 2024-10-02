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

;
;      NON-FUNCTIONAL RIGHT NOW BUT I AM TIRED
;

        .export         __strnsplit_int
        .import         popa, popax, pushax, aslax1
        .import         _strdup, _strchr

        .importzp       ptr1

        .include        "apple2.inc"

        .segment        "LOWCODE"

;int __fastcall__ _strnsplit_int(char in_place, char *in, char split, char **tokens, size_t max_tokens)
__strnsplit_int:
        sta       max_tokens
        stx       max_tokens+1
        jsr       popax
        sta       tokens
        stx       tokens+1
        jsr       popa
        sta       split
        jsr       popax
        sta       sep
        stx       sep+1
        sta       start
        stx       start+1
        sta       ptr1
        stx       ptr1+1
        jsr       popa
        sta       in_place

        lda       #$00
        sta       n_tokens

@next_sep:
        lda       sep
        ldx       sep+1
        jsr       pushax
        lda       split
        jsr       _strchr
        sta       sep
        stx       sep+1

        cpx       #$00          ; No need to check A, it's not in ZP
        beq       :+
        sta       ptr1
        stx       ptr1+1
        ldy       #$00
        tya
        sta       (ptr1),y

:       lda       n_tokens      ; Set ptr1 to tokens[n_tokens]
        ldx       n_tokens+1
        jsr       aslax1
        clc
        adc       tokens
        sta       ptr1
        txa
        adc       tokens+1
        sta       ptr1+1

        lda       start
        ldx       start+1

        bit       in_place
        bne       :+
        jsr       _strdup     ; strdup if needed

:       ldy       #$00        ; store to tokens[n_tokens]
        sta       (ptr1),y
        iny
        txa
        sta       (ptr1),y

        inc       n_tokens
        bne       :+
        inc       n_tokens+1
:       lda       n_tokens+1
        cmp       max_tokens+1
        bcc       @continue
        bne       @break
        lda       n_tokens
        cmp       max_tokens
        bcs       @break
@continue:
        ldx       sep+1
        beq       @break
        inc       sep
        bne       :+
        inx
:       lda       sep
        sta       start
        stx       sep+1
        stx       start+1
        jmp       @next_sep
@break:
        rts


        .bss

max_tokens: .res 2
tokens:     .res 2
split:      .res 1
sep:        .res 2
in_place:   .res 1
start:      .res 2
n_tokens:   .res 2
