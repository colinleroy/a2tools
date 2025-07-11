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
	.ifndef         __APPLE2ENH__
        .import         machinetype
	.endif
        .importzp       ptr1

        .include        "apple2.inc"

        .segment        "LOWCODE"

pop_and_store:
        pha
        jsr     popptr1
        pla
        sta     (ptr1),y
        rts

;void __fastcall__ get_scrollwindow(unsigned char *top, unsigned char *bottom)
_get_scrollwindow:
        jsr     pushax
        lda     WNDBTM
        jsr     pop_and_store
        lda     WNDTOP
        jmp     pop_and_store

;void __fastcall__ set_scrollwindow(unsigned char top, unsigned char bottom)
_set_scrollwindow:
        sta     WNDBTM
        jsr     popa
        sta     WNDTOP
do_vtabz:
        lda     CV
        jmp     FVTABZ

        .ifndef __APPLE2ENH__

; The non-enhanced //e ROM doesn't divide WNDLFT when in 80-cols
; mode, so it needs fixing to reflect the actual left boundary.
; Return with carry set if this is needed.
wndlft_needs_fixing:
        clc
        bit     machinetype
        bpl     :+             ; Is it a ][+?
        bvs     :+             ; //e. Enhanced?
        bit     RD80VID
        bpl     :+             ; Non-enhanced //e needs WNDLFT*2
        sec
:       rts

        .endif
;void __fastcall__ get_hscrollwindow(unsigned char *left, unsigned char *width)
_get_hscrollwindow:
        jsr     pushax
        lda     WNDWDTH
        jsr     pop_and_store
        lda     WNDLFT

        .ifndef __APPLE2ENH__
        jsr     wndlft_needs_fixing
        bcc     :+
        asl
:       .endif
        jmp     pop_and_store

;void __fastcall__ set_hscrollwindow(unsigned char left, unsigned char width) {
_set_hscrollwindow:
        sta     WNDWDTH
        jsr     popa
        .ifndef __APPLE2ENH__
        jsr     wndlft_needs_fixing
        bcc     :+
        lsr
:       .endif
        sta     WNDLFT
        jmp     do_vtabz
