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

        .export   _plot_dot, _plot_line

        .import   _hgr_hi, _hgr_low, _hgr_bit
        .import   _mod7_table, _div7_table
        .import   ox, oy, dx, dy

        .importzp ptr1

        .include  "helpers.inc"

; X,Y: x-y coordinates
.proc _plot_dot
        lda     _hgr_low,y
        clc
        adc     _div7_table,x
        sta     ptr1
        lda     _hgr_hi,y
        sta     ptr1+1
        ldy     #$00
        lda     _mod7_table,x
        tax
        lda     _hgr_bit,x
        ora     (ptr1),y
        sta     (ptr1),y
        rts
.endproc

.proc _plot_line
        lda     ox            ; Compute middle of line
        clc
        adc     dx
        ror
        sta     mid_x
        tax
        lda     oy
        clc
        adc     dy
        ror
        sta     mid_y
        tay
        jsr     _plot_dot     ; Plot at middle

        lda     mid_y         ; mid_y != dy?
        cmp     dy
        beq     :+
plot_line_start_to_middle:
        ; Backup coords before overwriting them
        lda     dx
        pha
        lda     dy
        pha
        ; and set end to middle
        lda     mid_x
        sta     dx
        lda     mid_y
        sta     dy

        ; plot line
        jsr     _plot_line

        ; recover middle from orig coords
        lda     dy
        sta     mid_y
        lda     dx
        sta     mid_x

        ; and restore original coords
        pla
        sta     dy
        pla
        sta     dx

        jmp     plot_line_check_end_to_middle

:       lda     mid_x
        cmp     dx            ; mid_x != dx?
        bne     plot_line_start_to_middle

plot_line_check_end_to_middle:
        lda     mid_y
        cmp     oy           ; mid_y != oy?
        beq     :+
plot_line_end_to_middle:
        ; Backup vars we're going to overwrite
        lda     ox
        pha
        lda     oy
        pha
        ; set end to middle
        lda     mid_x
        sta     ox
        lda     mid_y
        sta     oy
        jsr     _plot_line

        ; Restore vars
        pla
        sta     oy
        pla
        sta     ox
        rts                 ; we're done!

:       lda     mid_x
        cmp     ox          ; mid_x != ox ?
        bne     plot_line_end_to_middle
        rts
.endproc

.bss
mid_x:          .res 1
mid_y:          .res 1
