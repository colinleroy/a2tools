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

        .export         _file_select

        .export         _start_line, _end_line, _empty_line

        .import         _getfirstdevice, _getdevicedir, _getnextdevice
        .import         _dio_open, _dio_query_sectcount, _dio_close
        .import         _opendir, _readdir, _closedir, _dir_entry_count
        .import         _sprintf, _strcpy, _strcat, _cputs, _strlen, _strrchr
        .import         _malloc0, _free, _clreol, _clrzone, _revers

        .import         _gotoy, _gotox, _gotoxy, _wherex, _wherey
        .import         _cgetc, _cputc, _tolower, _beep
        .import         tosumula0, tosaddax, booleq
        .import         popa, popax, pusha0, pusha, pushax, swapstk, incaxy
        .import         return0, machinetype

        .import         CH_VLINE

        .importzp       _zp6, _zp8
        .include        "apple2.inc"

PATHNAME_MAX         = 65
PRODOS_FILENAME_MAX  = 17
PRODOS_MAX_VOLUMES   = 37
MAX_FILES            = 127

.struct FILE_ENTRY
  NAME                        .byte 17
  IS_DIR                      .byte
.endstruct

.proc _free_file_select_data
        lda       _file_entries
        ldx       _file_entries+1
        jsr       _free
        lda       #$00
        sta       _file_entries
        sta       _file_entries+1
        sta       num_entries
        sta       cur_sel
        sta       start
        rts
.endproc

.proc clr_file_select
        lda       sx
        jsr       pusha
        lda       sy
        jsr       pusha
        lda       #80                   ; clrzone checks bound
        jsr       pusha
        lda       ey
        jmp       _clrzone
.endproc

; Setup file_entries[A]
get_cur_entry_zp6:
        jsr       pusha0
        lda       #.sizeof(FILE_ENTRY)
        jsr       tosumula0
        jsr       pushax
        lda       _file_entries
        ldx       _file_entries+1
        jsr       tosaddax
        sta       _zp6
        stx       _zp6+1
        rts

inc_entry:
        inc       num_entries
        lda       num_entries
        jmp       get_cur_entry_zp6

.proc _get_device_entries
        jsr       _getfirstdevice
        sta       tmp_c

        lda       #<(.sizeof(FILE_ENTRY)*PRODOS_MAX_VOLUMES)
        ldx       #>(.sizeof(FILE_ENTRY)*PRODOS_MAX_VOLUMES)
        jsr       _malloc0
        sta       _file_entries
        stx       _file_entries+1
        sta       _zp6
        stx       _zp6+1

        lda       tmp_c
@get_device_dir:
        jsr       pusha
        .assert FILE_ENTRY::NAME = 0, error
        lda       _zp6
        ldx       _zp6+1
        jsr       pushax
        lda       #PRODOS_FILENAME_MAX+1
        jsr       _getdevicedir
        cpx       #$00
        bne       @prodos_dev

        ; Not a prodos device. What to do?
        .ifdef    FILESEL_ALLOW_NONPRODOS_VOLUMES
        lda       tmp_c
        jsr       _dio_open
        cmp       #$00
        beq       @next_dev

        jsr       pushax                ; Backup dev_handle
        jsr       _dio_query_sectcount
        jsr       swapstk               ; Backup blocks, get dev_handle
        jsr       _dio_close
        jsr       popax                 ; Get blocks
        cmp       #<280
        bne       @next_dev
        cpx       #>280
        bne       @next_dev

        ; tmp_c: 0000DSSS (as ProDOS, but shifted >>4)
        lda       _zp6
        ldx       _zp6+1
        jsr       pushax
        lda       #<SxDy_str
        ldx       #>SxDy_str
        jsr       pushax
        lda       tmp_c
        and       #$07
        jsr       pusha0
        ldy       #1
        lda       tmp_c
        and       #$08
        beq       :+
        iny
:       tya
        jsr       pusha0
        ldy       #$08
        jsr       _sprintf
        ; And fallback through prodos_dev
        ; To fill is_dir and iterate.
        .else
        jmp       @next_dev
        .endif
@prodos_dev:
        ldy       #FILE_ENTRY::IS_DIR   ; Store is_dir
        lda       #1
        sta       (_zp6),y
        jsr       inc_entry
@next_dev:
        lda       tmp_c
        jsr       _getnextdevice
        sta       tmp_c
        cmp       #$FF
        bne       @get_device_dir
        rts
.endproc

_zp8_is_dir:
        ldy       #$18
        lda       (_zp8),y
        cmp       #$0F
        rts

.proc _get_dir_entries
        lda       #<_last_dir
        ldx       #>_last_dir
        jsr       _opendir
        sta       d
        stx       d+1
        cpx       #$00
        beq       @return

        jsr       _dir_entry_count      ; Alloc struct
        cpx       #$00
        beq       :+

        lda       #MAX_FILES            ; Artificial limit on 127 files
        ldx       #$00

:       jsr       pushax
        lda       #<.sizeof(FILE_ENTRY)
        .assert .sizeof(FILE_ENTRY) < 256, error
        jsr       tosumula0
        jsr       _malloc0

        sta       _file_entries
        stx       _file_entries+1
        sta       _zp6
        stx       _zp6+1

@next_entry:
        lda       d
        ldx       d+1
        jsr       _readdir
        sta       _zp8
        stx       _zp8+1
        cpx       #$00
        beq       @closedir

        lda       dir_only              ; dir_only?
        beq       :+

        jsr       _zp8_is_dir           ; Yes so check DIRENT::D_TYPE
        bne       @next_entry

:       lda      _zp6
        ldx      _zp6+1
        jsr      pushax
        lda      _zp8                   ; DIRENT::D_NAME
        ldx      _zp8+1
        jsr      _strcpy

        jsr      _zp8_is_dir
        jsr      booleq
        ldy      #FILE_ENTRY::IS_DIR
        sta      (_zp6),y

        jsr       inc_entry
        lda       num_entries
        cmp       #MAX_FILES
        beq       @closedir             ; Artificial limit to 127 entries

        jmp       @next_entry
@closedir:
        lda       d
        ldx       d+1
        jsr       _closedir
@return:
        rts
.endproc

.proc _start_line
        lda       CH_VLINE
        jmp       _cputc
.endproc

.proc _end_line
        jsr       _clreol
        lda       #<EOL
        ldx       #>EOL
        jmp       _cputs
.endproc

.proc _empty_line
        jsr       _start_line
        jmp       _end_line
.endproc

push_last_dir:
        lda       #<_last_dir
        ldx       #>_last_dir
        jmp       pushax

load_last_dir_zp8:
        lda       #<_last_dir           ; Are we in a dir already?
        ldx       #>_last_dir
        sta       _zp8
        stx       _zp8+1
        ldy       #$00
        rts

.proc _enter_directory
        jsr       get_cur_entry_zp6

        ldy       #FILE_ENTRY::IS_DIR   ; Is this a dir?
        lda       (_zp6),y
        bne       :+
        rts

:       ldy       #$00
        lda       (_zp6),y
        cmp       #'/'
        beq       :+

        jsr       push_last_dir
        lda       #<slash_str
        ldx       #>slash_str
        jsr       _strcat

:       jsr       push_last_dir
        lda       _zp6
        ldx       _zp6+1
        jsr       _strcat
        lda       #1
        rts
.endproc

.proc _exit_directory
        lda       #<_last_dir
        ldx       #>_last_dir
        jsr       _strlen               ; Ignoring X because of pathname limits
        bne       :+
        rts

:       jsr       push_last_dir
        lda       #'/'
        jsr       _strrchr
        sta       _zp6
        stx       _zp6+1
        ldy       #$00
        tya
        sta       (_zp6),y
        lda       #1
        rts
.endproc

.proc _build_filename
        jsr       get_cur_entry_zp6
        ldy       #FILE_ENTRY::IS_DIR   ; Check that we have the kind we want
        lda       (_zp6),y
        cmp       dir_only
        beq       :+
        jmp       return0

:       lda       #<PATHNAME_MAX+1      ; Allocate filename
        ldx       #>PATHNAME_MAX+1
        jsr       _malloc0

        jsr       pushax                ; Push filename
        jsr       load_last_dir_zp8     ; Are we in a dir already?
        lda       (_zp8),y
        beq       :+

        ; Yes. Prepend it
        lda       _zp8                  ; Reload A
        jsr       _strcpy
        jsr       pushax                ; Repush filename for _strcat

        lda       #<slash_str
        ldx       #>slash_str
        jsr       _strcat
        jsr       pushax                ; Push for last strcat

:       lda       _zp6
        ldx       _zp6+1
        jmp       _strcat               ; Final strcat and return filename
.endproc

gotoxy_origin:
        lda       sy
        jsr       _gotoy
gotox_sx:
        lda       sx
        jmp       _gotox

.proc _file_select
        sta       prompt_str            ; Save parameters
        stx       prompt_str+1
        jsr       popa
        sta       dir_only

        lda       #7                    ; Setup "window"
        sta       loop_count
        jsr       _wherex
        sta       sx
        jsr       _wherey
        sta       sy
        clc
        adc       #3
        adc       loop_count
        sta       ey

        lda       #<please_wait_str
        ldx       #>please_wait_str
        jsr       _cputs

        jsr       load_last_dir_zp8     ; Returns with Y = 0

        lda       dir_only
        beq       @list_again

        tya                             ; If dir_only, zero last_dir
        sta       (_zp8),y

@list_again:
        jsr       _free_file_select_data; Free previous data

        jsr       load_last_dir_zp8     ; Check if last_dir != ""
        lda       (_zp8),y
        bne       :+
        jsr       _get_device_entries   ; If it is "", load devices
        jmp       @clear
:       jsr       _get_dir_entries      ; Else load last_dir's contents

@clear:
        jsr       clr_file_select
@disp_again:
        jsr       gotoxy_origin

        ; Print header
        jsr       _start_line
        lda       prompt_str
        ldx       prompt_str+1
        jsr       _cputs
        jsr       _end_line

        lda       num_entries
        bne       @calc_bounds

        ; Nothing to display
        jsr       gotox_sx
        jsr       _start_line
        lda       #<star_str
        ldx       #>star_str
        jsr       _cputs

        jsr       _start_line
        lda       dir_only
        beq       :+
        lda       #<no_dir_str
        ldx       #>no_dir_str
        bne       @disp_no_ent
:       lda       #<empty_str
        ldx       #>empty_str
@disp_no_ent:
        jsr       _cputs                ; Display emptyness
        jsr       _end_line
        jsr       gotox_sx
        jsr       _empty_line
        jsr       gotox_sx
        jsr       _start_line
        lda       #<any_key_up_str
        ldx       #>any_key_up_str
        jsr       _cputs
        jsr       _cgetc                ; Wait for keypress
        jmp       @up                   ; And go up

@calc_bounds:
        ; Calculate bounds
        lda       cur_sel               ; if (sel - loop_count >= start)
        sec
        sbc       loop_count
        cmp       start
        bmi       :+

        adc       #$00                  ; start = sel - loop_count + 1
        sta       start

:       lda       cur_sel               ; if (sel < start)
        cmp       start
        bpl       :+

        sta       start                 ; start = sel

:       lda       start                 ; stop = start + loop_count
        clc
        adc       loop_count
        sta       stop

        cmp       num_entries           ; if (num_entries < stop)
        bcc       :+

        lda       num_entries           ; stop = num_entries
        sta       stop

:       lda       start
        sta       i

@list_entries:
        lda       #$00                  ; Disable inverse
        jsr       _revers

        jsr       gotox_sx              ; Put vbar
        jsr       _start_line
        lda       #' '
        jsr       _cputc

        lda       i                     ; Set inverse
        cmp       cur_sel
        jsr       booleq
        jsr       _revers

        lda       i                     ; Get entry
        jsr       get_cur_entry_zp6
        jsr       _cputs

        ldy       #FILE_ENTRY::IS_DIR   ; Is it a dir?
        lda       (_zp6),y
        beq       :+
        lda       #'/'
        jsr       _cputc

:       jsr       _end_line

        inc       i                     ; i++
        lda       i
        cmp       stop
        bne       @list_entries         ; Do next entry

        ; Print footer
        lda       #$00
        jsr       _revers
        jsr       gotox_sx
        jsr       _empty_line
        jsr       gotox_sx
        jsr       _start_line
        lda       #<nav_str
        ldx       #>nav_str
        jsr       _cputs
        jsr       gotox_sx
        jsr       _start_line
        lda       #<enter_escape_str
        ldx       #>enter_escape_str
        jsr       _cputs

        ; Get key
        jsr       _cgetc
        jsr       _tolower
        sta       tmp_c

@check_r:
        cmp       #'r'
        bne       @check_ch_curs_right
        jmp       @list_again
@check_ch_curs_right:
        cmp       #$15                  ; CH_CURS_RIGHT
        bne       @check_ch_curs_left
        lda       cur_sel
        jsr       _enter_directory
        beq       @err_bell
        jmp       @list_again
@err_bell:
        jsr       _beep
        jmp       @disp_again

@check_ch_curs_left:
        cmp       #$08                  ; CH_CURS_LEFT
        bne       @check_ch_esc
@up:
        jsr       _exit_directory
        beq       @err_bell
        jmp       @list_again

@check_ch_esc:
        cmp       #$1B                  ; CH_ESC
        bne       @check_ch_enter
        lda       #$00
        tax
        jmp       @out

@check_ch_enter:
        cmp       #$0D                  ; CH_ENTER
        bne       @check_ch_curs_down
        lda       cur_sel
        jsr       _build_filename
        cpx       #$00
        beq       @err_bell
        jmp       @out

@check_ch_curs_down:
        cmp       #$0A
        beq       :+
        cmp       #'j'
        beq       :+
        cmp       #'J'
        bne       @check_ch_curs_up
:       inc       cur_sel
        lda       cur_sel
        cmp       num_entries
        bne       :+
        lda       #$00
        sta       cur_sel
:       jmp       @disp_again

@check_ch_curs_up:
        cmp       #$0B
        beq       :+
        cmp       #'u'
        beq       :+
        cmp       #'U'
        bne       @no_match
:       lda       cur_sel
        bne       :+

        ldy       num_entries
        dey
        sty       cur_sel
        jmp       @disp_again

:       dec       cur_sel
@no_match:
        jmp       @disp_again

@out:
        jsr       pushax                ; Save result (twice)
        jsr       pushax
        jsr       _free_file_select_data
        jsr       clr_file_select
        jsr       popax                 ; Pop result for print
        cpx       #$00
        beq       :+
        jsr       _cputs
:       jmp       popax                 ; Pop result for return
.endproc

        .rodata

SxDy_str:                     .asciiz "S%dD%d"
slash_str:                    .asciiz "/"
please_wait_str:              .asciiz "Please wait..."
no_dir_str:                   .asciiz "No directory*"
empty_str:                    .asciiz "Empty*"

star_str:                     .asciiz " *"
nav_str:                      .byte   "Arrows: nav; R: refresh", $0D,$0A,$00
any_key_up_str:               .asciiz "Any key to go up"
enter_escape_str:             .asciiz "Enter: choose; Esc: cancel"
EOL:                          .byte $0D,$0A,$00

        .bss

_last_dir:                    .res 65
_file_entries:                .res 2
d:                            .res 2
num_entries:                  .res 1
tmp_c:                        .res 1

sx:                           .res 1
sy:                           .res 1
ey:                           .res 1
dir_only:                     .res 1
prompt_str:                   .res 2

cur_sel:                      .res 1
start:                        .res 1
stop:                         .res 1
loop_count:                   .res 1
i:                            .res 1
