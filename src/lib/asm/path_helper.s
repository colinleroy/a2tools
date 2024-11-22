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

        .export         _register_start_device
        .export         _reopen_start_device

        .import         _chdir, _getcwd, _clrscr
        .import         _cgetc, _cputs, _gotoxy
        .import         _strrchr, _memcpy, _strlen

        .import         pusha, pushax, pushptr1, tosaddax
        .importzp       ptr1, tmp1

        .include        "apple2.inc"

CURPROG_ARGV0 := $280
FILENAME_MAX  := 1+15+64+1

        .segment "LC"

push_start_dir:
        lda       #<start_dir
        ldx       #>start_dir
        jmp       pushax

_register_start_device:
        ; Set start_dir as cur_end, in case we skip prefix
        lda       #<start_dir
        ldx       #>start_dir
        sta       ptr1
        stx       ptr1+1

        ; Is the current program pathname absolute?
        lda       CURPROG_ARGV0+1
        cmp       #'/'
        beq       @skip_prefix

        ; Get current PREFIX
        jsr       push_start_dir
        lda       #<FILENAME_MAX
        ldx       #>FILENAME_MAX
        jsr       _getcwd

        ; Go to the end
        jsr       push_start_dir
        jsr       _strlen
        jsr       tosaddax
        sta       ptr1
        stx       ptr1+1

        ; Is it a /?
        ldy       #0
        lda       #'/'
        cmp       (ptr1),y
        beq       :+
        
        ; Add a /
        sta       (ptr1),y
        inc       ptr1
        bne       :+
        inc       ptr1+1

:
@skip_prefix:
        ; Now, get the pathname of the current program
        ; It may contain relative directories and we want
        ; to keep them.

        ; Get the length for copy (it is not NULL-terminated)
        lda       CURPROG_ARGV0
        pha

        jsr       pushptr1
        lda       #<(CURPROG_ARGV0+1)
        ldx       #>(CURPROG_ARGV0+1)
        jsr       pushax
        pla
        ldx       #$00
        jsr       _memcpy
        sta       ptr1
        stx       ptr1+1

        ; Terminate the string
        ldy       CURPROG_ARGV0
        lda       #$00
        sta       (ptr1),y

        ; Cut the filename off
        jsr       push_start_dir
        lda       #'/'
        jsr       _strrchr
        sta       ptr1
        stx       ptr1+1
        lda       #$00
        tay
        sta       (ptr1),y
        rts

_reopen_start_device:
        lda       #<start_dir
        ldx       #>start_dir
        jsr       _chdir
        cmp       #$00
        bne       @prompt
        rts                             ; done!

@prompt:
        jsr       _clrscr
.ifdef __APPLE2ENH__
        lda       #((80-53)/2)
        jsr       pusha
        lda       #12
        jsr       _gotoxy

        lda      #<prompt_str
        ldx      #>prompt_str
        jsr      _cputs
.else
        lda       #((40-33)/2)
        jsr       pusha
        lda       #12
        jsr       _gotoxy

        lda      #<prompt_str1
        ldx      #>prompt_str1
        jsr      _cputs

        lda      #((40-19)/2)
        sta      CH
        lda      #<prompt_str2
        ldx      #>prompt_str2
        jsr      _cputs

.endif
        jsr      _cgetc
        jmp      _reopen_start_device

        .bss

start_dir: .res FILENAME_MAX+1

        .rodata

.ifdef __APPLE2ENH__
prompt_str: .asciiz "Please reinsert the program disk, then press any key."
.else
prompt_str1: .byte "Please reinsert the program disk,",$0D,$0A,$00
prompt_str2: .asciiz "then press any key."
.endif
