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

        .export   _init_caches
        .import   _backup_barsnd
        .import   _load_barsnd
        .import   _init_text_before_decompress
        .import   _backup_table_high
        .import   _load_table_high
        .import   _backup_bar_code
        .import   _bar_load_scores
        .import   _load_bar_code
        .import   _cache_working
        .import   _backup_bar_high
        .import   _load_bar_high
        .import   _text_mono40, _hgr_force_mono40
        .import   _home
        .import   _load_lc
        .import   _load_lowcode
        .import   _init_mouse
        .import   _load_hgr_mono_file
        .import   _print_error, _print_load_error

.segment "CODE"

no_mouse_str:     .byte "NO MOUSE DETECTED. PRESS A KEY TO EXIT."  ,$00

.proc _init_caches
        ; Init mouse first thing (it flickers HGR on IIplus)
        ; But disable IRQs as cc65's handler is in LOWCODE,
        ; which is not yet loaded. We will enable IRQs once we
        ; reach main, so that the mouse interrupts don't start to
        ; poke stuff in BSS still containing ONCE code.
        php
        sei

        jsr     _init_mouse
        bcc     :+                ; Do we have a mouse?

        lda     #<no_mouse_str
        ldx     #>no_mouse_str
        jmp     _print_error

:       ; Load LOWCODE (and splash screen)
        jsr     _load_hgr_mono_file

        jsr     _load_lowcode
        bcc     :+
        jmp     _print_load_error

:       ; We can now enable IRQs, the handler is loaded
        plp

        jsr     _home

        jsr     _hgr_force_mono40

        jsr     _load_lc
        bcc     :+
        jmp     _print_load_error

:       ; Preload assets at $4000 and back them up
        ; while we have the splash screen at $2000
        jsr     _load_bar_high
        bcc     :+
        jmp     _print_load_error

:       jsr     _backup_bar_high

        ; Don't bother preloading the rest if /RAM is unavailable
        lda     _cache_working
        beq     out

        jsr     _load_bar_code
        bcc     :+
        jmp     _print_load_error

:       jsr     _bar_load_scores
        jsr     _backup_bar_code

        jsr     _load_table_high
        bcc     :+
        jmp     _print_load_error

:       jsr     _backup_table_high

        jsr     _home

        ; Remove splashscreen as the intro sound
        ; overwrites it
        ; Ask data loader do it as late as possible
        lda     #1
        sta     _init_text_before_decompress
        jsr     _load_barsnd
        bcc     :+
        jmp     _print_load_error

:       jsr     _backup_barsnd
        lda     #0
        sta     _init_text_before_decompress
out:    rts
.endproc
