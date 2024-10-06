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
        .import         popa, popax, pushax, aslax1, pushptr1
        .import         _strdup, _strchr

        .importzp       ptr1, ptr2

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
        sta       ptr1
        stx       ptr1+1
        sta       start
        stx       start+1
        sta       ptr1
        stx       ptr1+1
        jsr       popa
        sta       in_place

        lda       #$00
        sta       n_tokens
        sta       n_tokens+1

@check_src:
        ldy       #$00
        lda       (ptr1),y
        beq       @done
        cmp       split
        bne       @inc_src

@insert_token:
        lda       n_tokens    ; Compute tokens[n_tokens]
        ldx       n_tokens+1
        jsr       aslax1
        clc
        adc       tokens
        sta       ptr2
        txa
        adc       tokens+1
        sta       ptr2+1

        jsr       pushptr1    ; Backup walker

        lda       start
        ldx       start+1
        bit       in_place
        bne       :+
        jsr       _strdup     ; Strdup if needed
:       ldy       #$00
        sta       (ptr2),y
        iny
        txa
        sta       (ptr2),y
        jsr       popax       ; Restore walker
        sta       ptr1
        stx       ptr1+1

        inc       n_tokens    ; Count token
        bne       :+
        inc       n_tokens+1

:       clc
        adc       #1
        sta       start       ; Update start
        bne       :+
        inx
:       stx       start+1

        lda       n_tokens    ; Check max tokens
        cmp       max_tokens
        bne       @inc_src
        lda       n_tokens+1
        cmp       max_tokens+1
        beq       @done
@inc_src:
        inc       ptr1
        bne       @check_src
        inc       ptr1+1
        bne       @check_src

@done:
        lda       start
        sta       ptr1
        lda       start+1
        sta       ptr1+1
        ldy       #$00
        lda       (ptr1),y
        beq       @really_done
        lda       n_tokens
        ldx       n_tokens+1
        clc
        adc       #1
        bne       :+
        inx
:       sta       max_tokens
        stx       max_tokens+1
        jmp       @insert_token
@really_done:
        lda       n_tokens
        ldx       n_tokens+1
        rts


        .bss

max_tokens: .res 2
tokens:     .res 2
split:      .res 1
in_place:   .res 1
start:      .res 2
n_tokens:   .res 2
