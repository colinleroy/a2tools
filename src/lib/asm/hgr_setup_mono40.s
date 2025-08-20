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
        .export  _load_hgr_mono_file
        .import  hgr_mono_file, _hgr_auxfile

        .import  _unlink, _close, popax, _write, _open, _memset, pushax, pusha0

        .destructor unlink_cached_files, 25
        .include "apple2.inc"
        .include "fcntl.inc"


HR1_OFF         := $C0B2
HR1_ON          := $C0B3
HR2_OFF         := $C0B4
HR2_ON          := $C0B5
HR3_OFF         := $C0B6
HR3_ON          := $C0B7
TEXT16_OFF      := $C0B8
TEXT16_ON       := $C0B9

; https://prodos8.com/docs/techref/writing-a-prodos-system-program/
; If your use involves hi-res graphics, you may protect those areas
; of auxiliary memory. If you save a dummy 8K file as the first
; entry in /RAM, it will always be saved at $2000 to $3FFF. If you
; then immediately save a second dummy 8K file to /RAM, it will be
; saved at $4000 to $5FFF. This protects the hi-res pages in auxiliary
; memory while maintaining /RAM as an online storage device.
;
; Up to the caller to make sure /RAM is empty at the time of the call.
.proc _load_hgr_mono_file
        sta     npages
        lda     #0
        sta     hgr_mono_file

        lda     #<hgr_data_buffer
        ldx     #>hgr_data_buffer
        jsr     pushax        ; Once for memset
        lda     #$F0
        ldx     #$00
        jsr     pushax
        lda     #<HGR_DATA_BUFSIZE
        ldx     #>HGR_DATA_BUFSIZE
        jsr     _memset

nextfile:
        lda     npages
        cmp     #2
        bne     secpage
        lda     #<_hgr_auxfile
        ldx     #>_hgr_auxfile
        bne     push
secpage:
        lda     #<hgr_fgbg2
        ldx     #>hgr_fgbg2
push:
        jsr     pushax
        lda      #<(O_WRONLY|O_CREAT)
        ldx      #$00
        jsr      pushax
        ldy      #$04
        jsr      _open
        cmp      #$FF
        beq      out

        sta     fd

:       dec     loop
        lda     fd
        jsr     pusha0
        lda     #<hgr_data_buffer
        ldx     #>hgr_data_buffer
        jsr     pushax
        lda     #<HGR_DATA_BUFSIZE
        ldx     #>HGR_DATA_BUFSIZE
        jsr     _write
        lda     loop
        bne     :-
        
        lda     fd
        ldx     #$00
        jsr     _close
        bne     out
        dec     npages
        bne     nextfile
        lda     #1
        sta     hgr_mono_file
out:    rts
.endproc

.proc unlink_cached_files
        lda       #<_hgr_auxfile
        ldx       #>_hgr_auxfile
        jsr       _unlink
        lda       #<hgr_fgbg2
        ldx       #>hgr_fgbg2
        jmp       _unlink
.endproc

        .data

hgr_fgbg2: .asciiz "/RAM/FGBG2"

        .bss

npages:          .res 1
fd:              .res 1
loop:            .res 1
HGR_DATA_BUFSIZE = 32
hgr_data_buffer: .res HGR_DATA_BUFSIZE
