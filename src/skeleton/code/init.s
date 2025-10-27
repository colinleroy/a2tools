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
        .import   _cache_working
        .import   _text_mono40, _hgr_force_mono40
        .import   _home
        .import   _load_lowcode, _load_data
        .import   _init_mouse
        .import   _load_hgr_mono_file
        .import   _print_error, _print_load_error

.segment "CODE"

no_mouse_str:     .byte "NO MOUSE DETECTED. PRESS A KEY TO EXIT."  ,$00

.proc _init_caches
        ; Load LOWCODE (and splash screen)
        lda     #1
        jsr     _load_hgr_mono_file

        jsr     _load_lowcode
        bcc     :+
        jmp     _print_load_error

:       jsr     _home

        jsr     _hgr_force_mono40

        jsr     _load_data

out:    ; Init mouse now (it flickers HGR on IIplus)
        jsr     _init_mouse
        bcc     :+                ; Do we have a mouse?

        lda     #<no_mouse_str
        ldx     #>no_mouse_str
        jmp     _print_error

:       rts
.endproc
