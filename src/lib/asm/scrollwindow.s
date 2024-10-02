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

        .export         _get_scrollwindow
        .export         _get_hscrollwindow
        .export         _set_scrollwindow
        .export         _set_hscrollwindow

        .import         popptr1, popa, pushax, FVTABZ
        .importzp       ptr1

        .include        "apple2.inc"

        .segment        "LOWCODE"

;void __fastcall__ get_scrollwindow(unsigned char *top, unsigned char *bottom)
_get_scrollwindow:
        sta       ptr1
        stx       ptr1+1
        ldy       #0
        lda       WNDBTM
        sta       (ptr1),y
        jsr       popptr1
        ldy       #0
        lda       WNDTOP
        sta       (ptr1),y
        rts

;void __fastcall__ set_scrollwindow(unsigned char top, unsigned char bottom)
_set_scrollwindow:
        sta      WNDBTM
        jsr      popa
        sta      WNDTOP
        lda      CV
        jmp      FVTABZ

;void __fastcall__ get_hscrollwindow(unsigned char *left, unsigned char *width)
_get_hscrollwindow:
        sta       ptr1
        stx       ptr1+1
        ldy       #0
        lda       WNDWDTH
        sta       (ptr1),y
        jsr       popptr1
        ldy       #0
        lda       WNDLFT
        sta       (ptr1),y
        rts

;void __fastcall__ set_hscrollwindow(unsigned char left, unsigned char width) {
_set_hscrollwindow:
        sta      WNDWDTH
        jsr      popa
        sta      WNDLFT
        lda      CV
        jmp      FVTABZ
