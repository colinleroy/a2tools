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
        .export _init_hgr
        .export _init_text
        .export _hgr_mixon
        .export _hgr_mixoff
        .export _hgr_init_done
        .export _hgr_mix_is_on

        .data

_hgr_init_done: .byte 0
_hgr_mix_is_on: .byte 0

        .segment "LOWCODE"

_init_hgr:
        .ifdef IIGS
        cmp       #$00
        beq       @not_monochrome

        lda       #$80
        sta       $C021       ; MONOCOLOR
        lda       $C029       ; NEWVIDEO
        ora       #$20        ; set bit 5
        sta       $C029
        bne       @color_set

@not_monochrome:
        lda       #$00
        sta       $C021       ; MONOCOLOR
        lda       $C029       ; NEWVIDEO
        and       #$DF        ; clear bit 5
        sta       $C029
        .endif

@color_set:
        lda       #$20
        sta       $E6         ; HGRPAGE

        ; Set HGR mode
        bit       $C000       ; 80STORE
        bit       $C050       ; TXTCLR
        bit       $C052       ; MIXCLR
        bit       $C057       ; HIRES
        bit       $C054       ; LOWSCR

        ldx       #1
        stx       _hgr_init_done
        dex
        stx       _hgr_mix_is_on
        rts

_init_text:
        bit       $C054       ; LOWSCR
        bit       $C051       ; TXTSET
        bit       $C056       ; LORES
        lda       #0
        sta       _hgr_init_done
        rts

_hgr_mixon:
        bit       $C053
        lda       #1
        sta       _hgr_mix_is_on
        rts

_hgr_mixoff:
        bit       $C052
        lda       #0
        sta       _hgr_mix_is_on
        rts
