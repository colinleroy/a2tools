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

        .export   _load_lowcode, _load_data
        .export   _init_text_before_decompress, _cache_working
        .export   _print_error, _print_load_error

        .import   _open, _read, _write, _close, _memmove, _unlink
        .import   pushax, popax, _text_mono40
        .import   __filetype, __auxtype

        .import   _strout
        .import   _read_key
        .import   _exit

        .import   __LOWCODE_START__, __LOWCODE_SIZE__
        .import   __SPLC_START__, __SPLC_SIZE__
        .import   __HGR_START__, __HGR_SIZE__
        .import   _decompress_zx02

        .importzp tmp1, ptr1

        .destructor unlink_files, 20

        .include  "apple2.inc"
        .include  "fcntl.inc"
        .include  "constants.inc"

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
        lda     #<__LOWCODE_START__
        ldx     #>__LOWCODE_START__
        sta     compressed_buf_start
        stx     compressed_buf_start+1
        jsr     pushax

        lda     #<(__LOWCODE_START__+__LOWCODE_SIZE__)
        ldx     #>(__LOWCODE_START__+__LOWCODE_SIZE__)
        sta     compressed_buf_end
        stx     compressed_buf_end+1

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

; A: data IO mode
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

        jsr     _decompress_zx02

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

.proc _load_lc
        lda     #<$4000
        ldx     #>$4000
        sta     compressed_buf_start
        stx     compressed_buf_start+1
        lda     #<$6000
        ldx     #>$6000
        sta     compressed_buf_end
        stx     compressed_buf_end+1

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

.segment "LOWCODE"

.proc _load_data
        jmp     _load_lc
.endproc

.proc unlink_files
        rts
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
load_err_str:        .asciiz "COULD NOT LOAD "
