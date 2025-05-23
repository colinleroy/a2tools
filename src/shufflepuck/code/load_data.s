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

        .export   _load_lowcode, _load_lc
        .export   _load_table_high, _backup_table, _backup_table_high, _restore_table
        .export   _load_opponent
        .export   _load_bar_high, _backup_bar_high, _restore_bar
        .export   _load_bar_code, _backup_bar_code, _restore_bar_code
        .export   _load_barsnd, _backup_barsnd, _restore_barsnd
        .export   _load_hall_of_fame, _load_scores, _save_scores
        .export   _init_text_before_decompress, _cache_working
        .export   _load_hgr_mono_file
        .export   _print_error, _print_load_error

        .import   hgr_mono_file
        .import   _open, _read, _write, _close, _memmove, _unlink
        .import   pushax, popax, _text_mono40, _memset
        .import   __filetype, __auxtype

        .import   _strout
        .import   _read_key
        .import   _exit

        .import   __LOWCODE_START__, __LOWCODE_SIZE__
        .import   __SPLC_START__, __SPLC_SIZE__
        .import   __HGR_START__, __HGR_SIZE__
        .import   __OPPONENT_START__, __OPPONENT_SIZE__
        .import   _decompress_lzsa1

        .importzp tmp1, ptr1

        .destructor unlink_cached_files, 20

        .include  "apple2.inc"
        .include  "fcntl.inc"
        .include  "constants.inc"
        .include  "scores.inc"

.segment "CODE"

.proc _print_load_error
        lda     #<load_err_str
        ldx     #>load_err_str
        jsr     _strout

        lda     filename
        ldx     filename+1
        ; Fallthrough to _print_error
.endproc

.proc _print_error
        jsr     _strout

        lda     lowcode_ok
        beq     :+
        jsr     _text_mono40
:       jsr     _read_key
        jmp     _exit
.endproc

.proc _load_lowcode
        jsr     set_compressed_buf_dynseg
        lda     #<__LOWCODE_START__
        ldx     #>__LOWCODE_START__
        jsr     pushax
        lda     #<__LOWCODE_SIZE__
        ldx     #>__LOWCODE_SIZE__
        jsr     pushax
        lda     #<lowcode_name
        ldx     #>lowcode_name

        jsr     read_compressed
        bcs     :+
        inc     lowcode_ok
:       rts
.endproc

.proc _load_hgr_mono_file
        lda     hgr_mono_file
        beq     :+
        rts

:       jsr     push_hgr_start        ; Push once for file_io_at size
        jsr     push_hgr_start        ; Once for file_io_at buffer
        jsr     push_hgr_start        ; Once for memset dest
        lda     #$F0                  ; FG white, BG black
        ldx     #$00
        jsr     pushax
        lda     #<$2000
        ldx     #>$2000
        jsr     _memset

        lda     #<hgr_fgbg
        ldx     #>hgr_fgbg
        jsr     set_noncompressed
        ldy     #(O_WRONLY|O_CREAT)
        jsr     file_io_at
        bcs     :+
        lda     #1
        sta     hgr_mono_file
:       rts
.endproc

.proc read_compressed
        ldy     #$01              ; O_RDONLY
        sty     do_uncompress

        jmp     file_io_at
.endproc

.proc set_noncompressed
        ldy     #$00
        sty     do_uncompress
        rts
.endproc

; The minimum to load lowcode
.proc set_compressed_buf_hgr
        lda     #<__HGR_START__
        ldx     #>__HGR_START__
        sta     compressed_buf_start
        stx     compressed_buf_start+1
        lda     #<(__HGR_START__+__HGR_SIZE__)
        ldx     #>(__HGR_START__+__HGR_SIZE__)
        sta     compressed_buf_end
        stx     compressed_buf_end+1
        rts
.endproc

.proc set_compressed_buf_dynseg
        lda     #<__OPPONENT_START__
        ldx     #>__OPPONENT_START__
        sta     compressed_buf_start
        stx     compressed_buf_start+1
        lda     #<(__OPPONENT_START__+__OPPONENT_SIZE__)
        ldx     #>(__OPPONENT_START__+__OPPONENT_SIZE__)
        sta     compressed_buf_end
        stx     compressed_buf_end+1
        rts
.endproc

.proc push_hgr_start
        lda     #<__HGR_START__
        ldx     #>__HGR_START__
        jmp     pushax
.endproc

.proc push_opponent_start
        lda     #<__OPPONENT_START__
        ldx     #>__OPPONENT_START__
        jmp     pushax
.endproc

.proc push_hgr_size
        lda     #<__HGR_SIZE__
        ldx     #>__HGR_SIZE__
        jmp     pushax
.endproc

.proc push_hgr_page_buf
        jsr     push_hgr_start
        jmp     push_hgr_size
.endproc

.proc push_high_hgr_page_buf
        jsr     push_opponent_start
        jmp     push_hgr_size
.endproc

.proc push_dynseg_page_buf
        jsr     push_opponent_start
        lda     #<__OPPONENT_SIZE__
        ldx     #>__OPPONENT_SIZE__
        jmp     pushax
.endproc

.proc push_extra_large_page_buf
        jsr     push_hgr_start
        lda     #<(__HGR_SIZE__+__OPPONENT_SIZE__)
        ldx     #>(__HGR_SIZE__+__OPPONENT_SIZE__)
        jmp     pushax
.endproc

; A: data IO mode
; X: Whether to uncompress data
.proc _data_io
        lda     io_mode
        cmp     #<O_RDONLY        ; Patch io function (_read or _write)
        beq     set_read
        lda     #<_write          ; Patch for _write
        ldx     #>_write
        bne     :+                ; BRA
set_read:
        lda     #<_read           ; Patch for _read
        ldx     #>_read

:       sta     data_io_func+1    ; Store patched io function
        stx     data_io_func+2

do_io:
        ; Open file
        lda     #$06          ; Set filetype PRODOS_T_BIN
        sta     __filetype
        lda     #$00
        sta     __auxtype

        lda     filename      ; Get filename and push it for _open
        ldx     filename+1
        jsr     pushax

        lda     io_mode
        ldx     #0
        jsr     pushax        ; Push mode for _open

        ldy     #$04          ; _open is variadic, tell it how many bytes it
        jsr     _open         ; can pop
        cmp     #$FF
        bne     :+
        sec                   ; Could not open file - signal error with carry
        rts

:       jsr     pushax        ; We now have an fd. Push it for _read/_write
        jsr     pushax        ; and a second time for _close

        ldx     do_uncompress ; Do we need to uncompress?
        bne     :+
        lda     destination   ; No, so read directly where we want it
        ldx     destination+1
        bne     push_dest     ; (Always branch)
:
        lda     compressed_buf_start
        ldx     compressed_buf_start+1
push_dest:
        jsr     pushax

        lda     size
        ldx     size+1
data_io_func:
        jsr     $FFFF         ; Patched. Calls _read or _write
        sta     tmp1          ; Remember size for uncompress
        stx     tmp1+1

        jsr     popax         ; Get fd back from stack
        jsr     _close        ; And close the file

        ldx     do_uncompress ; Do we need to uncompress?
        bne     uncompress
        clc                   ; No, so we're done.
        rts

uncompress:
        ; Compute where to move data, we want it to be
        ; the furthest possible in the available buffer
        ; so that decompression doesn't overwrite the last
        ; compressed bytes
        lda     compressed_buf_end
        sec
        sbc     tmp1
        tay

        lda     compressed_buf_end+1
        sbc     tmp1+1
        tax
        tya
        jsr     pushax        ; Push it for decompressor source
        jsr     pushax        ; and for memmove dest

        ; Where to move data from (the compressed buffer)
        lda     compressed_buf_start
        ldx     compressed_buf_start+1
        jsr     pushax        ; Push source for memmove

        lda     tmp1          ; Copy compressed size bytes
        ldx     tmp1+1
        jsr     _memmove

finish_decompress:
        lda     _init_text_before_decompress
        beq     :+
        jsr     _text_mono40

:       lda     destination       ; Push user-specified destination buffer
        ldx     destination+1

        bit       $C083           ; Enable writing to LC
        bit       $C083           ; In case we're writing to it

        jsr     _decompress_lzsa1

        bit     $C080

        clc                   ; And we're done!
        rts
.endproc

; Filename in AX, R/W in Y. destination in TOS.
; set do_uncompress before.

.proc file_io_at
        sta     filename
        stx     filename+1
        sty     io_mode

        jsr     popax
        sta     size
        stx     size+1

        jsr     popax
        sta     destination
        stx     destination+1

        jmp     _data_io
.endproc

.segment "LOWCODE"

.proc _load_lc
        jsr     set_compressed_buf_dynseg

        lda     #<__SPLC_START__
        ldx     #>__SPLC_START__
        jsr     pushax
        lda     #<__SPLC_SIZE__
        ldx     #>__SPLC_SIZE__
        jsr     pushax
        lda     #<lc_name
        ldx     #>lc_name

        jmp     read_compressed
.endproc

; A = which opponent to load
.proc _load_opponent
        ; Set correct filename for level
        clc
        adc     #'A'
        sta     opponent_name_tmpl+9

        jsr     set_compressed_buf_dynseg
        jsr     push_dynseg_page_buf
        lda     #<opponent_name_tmpl
        ldx     #>opponent_name_tmpl

        jmp     read_compressed
.endproc

.proc _load_table_high
        jsr     set_compressed_buf_dynseg
        jsr     push_dynseg_page_buf
        jmp     load_table_at
.endproc

.proc load_table_hgr
        jsr     set_compressed_buf_hgr
        jsr     push_hgr_page_buf
.endproc

.proc load_table_at
        lda     #<table_name
        ldx     #>table_name

        jmp     read_compressed
.endproc

.proc _load_bar_high
        jsr     set_compressed_buf_dynseg
        jsr     push_dynseg_page_buf
        jmp     load_bar_at
.endproc

.proc load_bar_hgr
        jsr     set_compressed_buf_hgr
        jsr     push_hgr_page_buf
.endproc

.proc load_bar_at
        lda     #<bar_name
        ldx     #>bar_name

        jmp     read_compressed
.endproc

.proc _load_bar_code
        jsr     set_compressed_buf_dynseg
        jsr     push_dynseg_page_buf
        lda     #<bar_code_name
        ldx     #>bar_code_name

        jmp     read_compressed
.endproc

.proc _load_barsnd
        jsr     set_compressed_buf_dynseg
        jsr     push_extra_large_page_buf
        lda     #<barsnd_name
        ldx     #>barsnd_name

        jmp     read_compressed
.endproc

.proc set_table_backup_file
        lda     #<table_backup_name
        ldx     #>table_backup_name
        jmp     set_noncompressed
.endproc

.proc set_bar_backup_file
        lda     #<bar_backup_name
        ldx     #>bar_backup_name
        jmp     set_noncompressed
.endproc


.proc set_table_backup_params
        jsr     push_hgr_page_buf
        jmp     set_table_backup_file
.endproc

.proc backup_io
        jsr      file_io_at
        bcc      :+
        lda      #0
        sta      _cache_working
:       rts
.endproc

.proc _backup_table
        lda      _cache_working
        beq      :+
        jsr      set_table_backup_params
        ldy      #(O_WRONLY|O_CREAT)
        jmp      backup_io

:       sec
        rts
.endproc

.proc _backup_table_high
        jsr     push_high_hgr_page_buf
        jsr     set_table_backup_file
        ldy     #(O_WRONLY|O_CREAT)
        jmp     backup_io
.endproc

.proc _restore_table
        lda     _cache_working
        beq     no_cache
        jsr     set_table_backup_params
        ldy     #O_RDONLY
        jsr     file_io_at
        bcs     no_cache
        rts
no_cache:
        jmp     load_table_hgr
.endproc

.proc set_bar_backup_params
        jsr     push_hgr_page_buf
        jmp     set_bar_backup_file
.endproc

.proc _backup_bar_high
        jsr     push_high_hgr_page_buf
        jsr     set_bar_backup_file
        ldy     #(O_WRONLY|O_CREAT)
        jmp     backup_io
.endproc

.proc _restore_bar
        lda     _cache_working
        beq     no_cache
        jsr     set_bar_backup_params
        ldy     #O_RDONLY
        jsr     file_io_at
        bcs     no_cache
        rts
no_cache:
        jmp     load_bar_hgr
.endproc

.proc set_bar_code_backup_params
        jsr     push_dynseg_page_buf
        lda     #<bar_code_backup_name
        ldx     #>bar_code_backup_name
        jmp     set_noncompressed
.endproc

.proc _backup_bar_code
        jsr      set_bar_code_backup_params
        ldy      #(O_WRONLY|O_CREAT)
        jmp      backup_io
.endproc

.proc _restore_bar_code
        lda     _cache_working
        beq     no_cache
        jsr     set_bar_code_backup_params
        ldy     #O_RDONLY
        jsr     file_io_at
        bcs     no_cache
        rts
no_cache:
        jmp     _load_bar_code
.endproc

.proc set_barsnd_backup_params
        jsr     push_extra_large_page_buf
        lda     #<barsnd_backup_name
        ldx     #>barsnd_backup_name
        jmp     set_noncompressed
.endproc

.proc _backup_barsnd
        jsr      set_barsnd_backup_params
        ldy      #(O_WRONLY|O_CREAT)
        jmp      backup_io
.endproc

.proc _restore_barsnd
        lda     _cache_working
        beq     no_cache
        jsr     set_barsnd_backup_params
        ldy     #O_RDONLY
        jsr     file_io_at
        bcs     no_cache
        rts
no_cache:
        jmp     _load_barsnd
.endproc

.proc unlink_cached_files
        jsr     set_table_backup_file
        jsr     _unlink
        jsr     set_bar_backup_file
        jsr     _unlink
        lda     #<bar_code_backup_name
        ldx     #>bar_code_backup_name
        jsr     _unlink
        lda     #<hgr_fgbg
        ldx     #>hgr_fgbg
        jsr     _unlink
        lda     #<barsnd_backup_name
        ldx     #>barsnd_backup_name
        jmp     _unlink
.endproc

        .bss

filename:             .res 2
destination:          .res 2
tmp_destination:      .res 2
size:                 .res 2
do_uncompress:        .res 1
io_mode:              .res 1
compressed_buf_start: .res 2
compressed_buf_end:   .res 2
lowcode_ok:           .res 0

        .data

_init_text_before_decompress: .byte 0
_cache_working:      .byte 1
lowcode_name:        .asciiz "LOW.CODE"
lc_name:             .asciiz "LC.CODE"
table_name:          .asciiz "TABLE.IMG"
bar_name:            .asciiz "BAR.IMG"
bar_code_name:       .asciiz "BAR.CODE"
barsnd_name:         .asciiz "BAR.SND"
table_backup_name:   .asciiz "/RAM/TABLE.IMG"
bar_backup_name:     .asciiz "/RAM/BAR.IMG"
bar_code_backup_name:.asciiz "/RAM/BAR.CODE"
barsnd_backup_name:  .asciiz "/RAM/BAR.SND"
opponent_name_tmpl:  .asciiz "OPPONENT.X"
hgr_fgbg:            .asciiz "/RAM/FGBG"
load_err_str:        .asciiz "COULD NOT LOAD "

.segment "barcode"
; --- only accessible from the bar code segment
; --- don't use it from elsewhere
.proc set_scores_params
        jsr     pushax            ; Push buffer address
        lda     #<SCORE_TABLE_SIZE
        ldx     #>SCORE_TABLE_SIZE
        jsr     pushax
        jsr     set_noncompressed
        lda     #<scores_name
        ldx     #>scores_name
        rts
.endproc

.proc _load_scores
        jsr      set_scores_params
        ldy      #(O_RDONLY)
        jmp      file_io_at
.endproc

.proc _save_scores
        jsr      set_scores_params
        ldy      #(O_WRONLY|O_CREAT)
        jmp      file_io_at
.endproc

.proc _load_hall_of_fame
        jsr     set_compressed_buf_hgr
        jsr     push_hgr_page_buf
        lda     #<hallfame_name
        ldx     #>hallfame_name
        jmp     read_compressed
.endproc

scores_name:         .asciiz "SCORES"
hallfame_name:       .asciiz "HALLFAME.IMG"
