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

        .export   _load_table, _backup_table, _restore_table
        .export   _load_lowcode, _load_opponent

        .import   _open, _read, _write, _close, _memcpy
        .import   pushax, popax
        .import   __filetype, __auxtype

        .import   __LOWCODE_START__, __LOWCODE_SIZE__
        .import   __HGR_START__, __HGR_SIZE__
        .import   __OPPONENT_START__, __OPPONENT_SIZE__
        .import   _decompress_lz4

        .importzp tmp1, ptr1

        .include  "apple2.inc"
        .include  "fcntl.inc"

.segment "LC"


; A = which opponent to load
.proc _load_opponent
        pha

        bit       $C083           ; Enable writing to LC
        bit       $C083           ; So we can patch our code

        lda       #<opponent_name_tmpl
        sta       filename
        lda       #>opponent_name_tmpl
        sta       filename+1

        ; Set correct filename for level
        pla
        clc
        adc       #'A'
        sta       opponent_name_tmpl

        lda      #<__OPPONENT_SIZE__
        sta      size
        lda      #>__OPPONENT_SIZE__
        sta      size+1

        lda      #<__OPPONENT_START__
        sta      destination
        lda      #>__OPPONENT_START__
        sta      destination+1

        lda      #<O_RDONLY
        ldx      #$01
        jmp      _data_io
.endproc

.proc set_hgr_destination_buffer
        lda      #<__HGR_SIZE__
        sta      size
        lda      #>__HGR_SIZE__
        sta      size+1

        lda      #<__HGR_START__
        sta      destination
        lda      #>__HGR_START__
        sta      destination+1
        rts
.endproc

.proc _load_table
        bit       $C083           ; Enable writing to LC
        bit       $C083           ; So we can patch our code
        lda       #<table_name
        sta       filename
        lda       #>table_name
        sta       filename+1

        jsr      set_hgr_destination_buffer

        lda      #<O_RDONLY
        ldx      #$01
        ; Fallthrough to _data_io
.endproc

; A: data IO mode
; X: Whether to uncompress data
.proc _data_io
        stx     do_uncompress     ; Save whether we need to uncompress lz4
        sta     data_io_mode+1    ; Patch _open mode

        cmp     #<O_RDONLY        ; Patch io function (_read or _write)
        beq     set_read
        lda     #<_write          ; Patch for _write
        ldx     #>_write
        jmp     setup_out_buf
set_read:
        lda     #<_read           ; Patch for _read
        ldx     #>_read

setup_out_buf:
        sta     data_io_func+1    ; Store patched io function
        stx     data_io_func+2

        lda     destination       ; Patch buffer end
        clc
        adc     size
        sta     buf_end_low+1
        lda     destination+1
        adc     size+1
        sta     buf_end_high+1

        ldy     destination       ; No. Do our IO directly to/from the
        sty     file_io_low+1     ; user-provided buffer
        ldy     destination+1
        sty     file_io_high+1

do_io:
        ; Open file
        lda     #$06          ; Set filetype PRODOS_T_BIN
        sta     __filetype
        lda     #$00
        sta     __auxtype

        lda     filename      ; Get filename and push it for _open
        ldx     filename+1
        jsr     pushax

data_io_mode:
        lda     #$FF          ; Patched with O_RDONLY or O_WRONLY
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

file_io_low:
        lda     #$FF          ; Push IO buffer to _read/_write.
file_io_high:
        ldx     #$FF
        jsr     pushax

        lda     size          ; Set size for _read/_write
        ldx     size+1
data_io_func:
        jsr     $FFFF         ; Patched. Calls _read or _write

        jsr     popax         ; Get fd back from stack
        jsr     _close        ; And close the file

        ldx     do_uncompress ; Do we need to uncompress?
        bne     uncompress
        clc                   ; No, so we're done.
        rts

uncompress:
        lda     destination
        sta     ptr1
        lda     destination+1
        sta     ptr1+1

        ldy     #0
        lda     (ptr1),y      ; Get compressed size (the first two bytes
        sta     tmp1          ; of the compressed data we just read)
        iny
        lda     (ptr1),y
        sta     tmp1+1

        ; Get uncompressed size (the next two bytes)
        iny
        lda     (ptr1),y
        sta     size
        iny
        lda     (ptr1),y
        sta     size+1

        ; Compute where to move data, we want it to be
        ; the furthest possible in the available buffer
        ; so that decompression doesn't overwrite the last
        ; compressed bytes
buf_end_low:
        lda     #$FF
        sec
        sbc     tmp1
        tay
buf_end_high:
        lda     #$FF
        sbc     tmp1+1
        tax
        tya
        jsr     pushax        ; Push it for decompressor source
        jsr     pushax        ; and for memcpy dest

        ; Where to move data from (the compressed buffer excluding the 4
        ; header bytes)
        lda     ptr1
        clc
        adc     #4
        ldx     ptr1+1
        bcc     :+
        inx
:       jsr     pushax        ; Push source for memcpy

        lda     tmp1          ; Copy compressed size bytes
        ldx     tmp1+1
        jsr     _memcpy

        lda     destination   ; Push user-specified destination buffer
        ldx     destination+1
        jsr     pushax

        lda     size          ; Inform lz4 decompressor of the uncompressed size
        ldx     size+1        ; so it knows when to stop

        jsr     _decompress_lz4
        clc                   ; And we're done!
        rts

.endproc

.proc _load_lowcode
        bit       $C083           ; Enable writing to LC
        bit       $C083
        lda       #<lowcode_name
        sta       filename
        lda       #>lowcode_name
        sta       filename+1

        lda      #<__LOWCODE_START__
        sta      destination
        lda      #>__LOWCODE_START__
        sta      destination+1

        lda      #<__LOWCODE_SIZE__
        sta      size
        lda      #>__LOWCODE_SIZE__
        sta      size+1

        lda      #<O_RDONLY
        ldx      #$01
        jmp      _data_io
.endproc

.proc set_table_backup_params
        lda     #<table_backup_name
        sta     filename
        lda     #>table_backup_name
        sta     filename+1

        jmp      set_hgr_destination_buffer
.endproc

.proc _backup_table
        jsr      set_table_backup_params
        lda      #(O_WRONLY|O_CREAT)
        ldx      #$00
        jmp      _data_io
.endproc

.proc _restore_table
        jsr      set_table_backup_params
        lda      #O_RDONLY
        ldx      #$00
        jsr      _data_io
        bcs      no_cache
        rts
no_cache:
        jmp      _load_table
.endproc

        .bss

filename:      .res 2
destination:   .res 2
size:          .res 2
do_uncompress: .res 1

        .data

lowcode_name:        .asciiz "LOWCODE"
table_name:          .asciiz "TABLE"
table_backup_name:   .asciiz "/RAM/TABLE"
opponent_name_tmpl:  .asciiz "X"
