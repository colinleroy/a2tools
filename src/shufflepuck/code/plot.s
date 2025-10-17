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

_plot_line = _plot_line_bresenham

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

.proc _plot_line_bresenham
        lda     ox            ; Copy parameters
        sta     _x0
        lda     oy
        sta     _y0
        lda     dx
        sta     _x1
        lda     dy
        sta     _y1

set_x_params:
        lda     _x0           ; Setup deltax and stepx
        cmp     _x1
        bcs     negdx
posdx:  lda     _x1
        sec
        sbc     _x0
        ldy     #$EE          ; INC $nnnn
        jmp     finish_x_params
negdx:  ; carry already set, _x0 already in A
        sbc     _x1
        ldy     #$CE          ; DEC $nnnn

finish_x_params:
        sta     _deltax
        sty     stepx

        lda     _y0           ; Setup deltay and stepy
        cmp     _y1
        bcs     negdy
posdy:
        sec
        sbc     _y1
        ldy     #$EE          ; INC $nnnn
        bne     finish_y_params
negdy:  lda     _y1
        ; sec - carry already set
        sbc     _y0
        ldy     #$CE          ; DEC $nnnn

finish_y_params:
        ldx     #$00
        bcs     :+
        dex
:       sta     _deltay
        stx     _deltay+1
        sty     stepy

        lda     _deltax       ; Setup error counter
        clc
        adc     _deltay
        sta     _b_err
        lda     #$00
        adc     _deltay+1
        sta     _b_err+1

next_dot:
        ldx     _x0           ; Plot a dot
        ldy     _y0
        jsr     _plot_dot

        lda     _x0           ; Are we done?
        cmp     _x1
        bne     :+
        lda     _y0
        cmp     _y1
        beq     out           ; We're done!

:       lda     _b_err+1
        sta     _b_err2+1
        lda     _b_err        ; Update err2 = err<<1
        asl
        sta     _b_err2
        rol     _b_err2+1

check_deltay:
        cmp     _deltay       ; err2 >= deltay?
        lda     _b_err2+1
        sbc     _deltay+1
        bvs     :+
        eor     #$80
:       asl
        bcc     check_deltax
        clc
        lda     _b_err        ; Yes. err += deltay,
        adc     _deltay
        sta     _b_err
        lda     _deltay+1
        adc     _b_err+1
        sta     _b_err+1

stepx:
        BIT     _x0           ; x0 += stepx (BIT patched with INC/DEC)

check_deltax:
        lda     _deltax       ; deltax >= err2?
        cmp     _b_err2
        lda     #$00
        sbc     _b_err2+1
        bvs     :+
        eor     #$80
:       asl
        bcc     next_dot      ; No. Go do next dot
        clc
        lda     _b_err        ; Yes. err += deltax
        adc     _deltax
        sta     _b_err
        lda     #$00
        adc     _b_err+1
        sta     _b_err+1

stepy:
        BIT     _y0           ; y0 += stepy (patched with INC/DEC)
        jmp     next_dot
out:    rts
.endproc

.bss
_x0:            .res 1        ; _plot_line_bresenham params
_x1:            .res 1
_y0:            .res 1
_y1:            .res 1
_deltax:        .res 1
_deltay:        .res 2        ; Those must be 16 bits otherwise
_b_err:         .res 2        ; they overflow all the way
_b_err2:        .res 2
