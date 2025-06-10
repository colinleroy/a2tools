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

        .export   _plot_line_recursive

        .import   _plot_dot

        .import   pushax, _bzero

        .import   _hgr_hi, _hgr_low, _hgr_bit
        .import   _mod7_table, _div7_table

        .import   ox, oy, dx, dy

        .importzp ptr1

.proc _plot_line_recursive
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
.ifdef INLINE_PLOT
        lda     _hgr_low,y
        sta     ptr1
        lda     _hgr_hi,y
        sta     ptr1+1

        lda     _div7_table,x
        tay
        lda     _mod7_table,x
        tax
        lda     _hgr_bit,x
        ora     (ptr1),y
        sta     (ptr1),y
.else
        jsr     _plot_dot     ; Plot at middle
.endif

        lda     mid_y         ; mid_y != dy?
        cmp     oy
        beq     :+
plot_line_start_middle:
        ; Backup coords before overwriting them
        lda     ox
        pha
        lda     oy
        pha
        ; and set end to middle
        lda     mid_x
        sta     ox
        lda     mid_y
        sta     oy

        ; plot line
        jsr     _plot_line_recursive

        ; recover middle from orig coords
        lda     oy
        sta     mid_y
        lda     ox
        sta     mid_x

        ; and restore original coords
        pla
        sta     oy
        pla
        sta     ox

        jmp     plot_line_check_end_middle

:       lda     mid_x
        cmp     ox            ; mid_x != ox?
        bne     plot_line_start_middle

plot_line_check_end_middle:
        lda     mid_y
        cmp     dy           ; mid_y != dy?
        beq     :+
plot_line_end_middle:
        ; Backup vars we're going to overwrite
        lda     dx
        pha
        lda     dy
        pha
        ; set end to middle
        lda     mid_x
        sta     dx
        lda     mid_y
        sta     dy
        jsr     _plot_line_recursive

        ; Restore vars
        pla
        sta     dy
        pla
        sta     dx
        jmp     plot_done

:       lda     mid_x
        cmp     dx          ; mid_x != dx ?
        bne     plot_line_end_middle

plot_done:
        rts
.endproc

.bss
mid_x: .res 1
mid_y: .res 1
