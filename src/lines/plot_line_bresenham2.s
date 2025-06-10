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

        .export   _plot_line_bresenham2

        .import   _plot_dot

        .import   pushax, _bzero

        .import   _hgr_hi, _hgr_low, _hgr_bit
        .import   _mod7_table, _div7_table

        .import   dx, dy, ox, oy
        
        .importzp ptr1, tmp1, tmp2, ptr2, ptr3, ptr4

_x0       = ptr2
_x1       = ptr2+1
_y0       = ptr3
_y1       = ptr3+1
error     = ptr4
slope_err = ptr4+1

.proc get_slope_err
        stx     tmp1
        ldx     #1
        stx     tmp2

shift:
        asl
        bcs     sub
        cmp     tmp1
        bcc     check

sub:
        sbc     tmp1
        sec

check:
        rol     tmp2
        bcc     shift

        lda     tmp2
        rts
.endproc

.proc _plot_line_bresenham2
        lda     ox                ; Figure out if we need to invert start/end
        cmp     dx
        bcc     invert_od
normal_od:                        ; No
        lda     dx
        sta     _x0
        lda     dy
        sta     _y0
        lda     oy
        sta     _y1
        lda     ox
        sta     _x1
        jmp     get_deltax
invert_od:                        ; Yes
        lda     ox
        sta     _x0
        lda     oy
        sta     _y0
        lda     dy
        sta     _y1
        lda     dx
        sta     _x1

get_deltax:                       ; Compute deltax (x1 in A)
        sec
        sbc     _x0
        sta     _deltax

        lda     _y1               ; Compute deltay, determine quadrant
        cmp     _y0
        bcc     neg_y
pos_y:                            ; Deltay positive, store it and use iny
        sbc     _y0
        sta     _deltay
        lda     #$C8              ; INY
        jmp     set_ydir
neg_y:                            ; Deltay negative, make it positive and dey
        lda     _y0
        sec
        sbc     _y1
        sta     _deltay
        lda     #$88              ; DEY

set_ydir:
        sta     move_y1           ; Patch code witch iny/dey
        sta     move_y2

.ifdef INLINE_PLOT              ; Init hgr pointer
        ldy     _y0
        lda     _hgr_low,y
        sta     ptr1
        lda     _hgr_hi,y
        sta     ptr1+1

        ldx     _x0
        lda     _div7_table,x     ; Init X values
        sta     xdiv7a+1
        sta     xdiv7b+1
        lda     _mod7_table,x
        tax
        lda     _hgr_bit,x
        sta     xbita+1
        sta     xbitb+1
.endif

        lda     #$80              ; Init error
        sta     error

        lda     _deltax           ; Determine slope
        cmp     _deltay
        bcc     slope_y

slope_x:
        lda     _deltay           ; Compute slope_err
        ldx     _deltax
        jsr     get_slope_err
        sta     slope_err

        clc                       ; For later error += slope_err

        ldx     _x0
next_x:
        stx     _x0
.ifdef INLINE_PLOT
xdiv7a:
        ldy     #$FF
xbita:
        lda     #$FF
        ora     (ptr1),y
        sta     (ptr1),y
.else
        ldy     _y0
        jsr     _plot_dot
.endif

        lda     error             ; Update error
        adc     slope_err
        sta     error
        bcc     :+                ; If overflowed, update Y
        ldy     _y0
move_y1:
        NOP                       ; Update Y (patched with iny/dey)
.ifdef INLINE_PLOT
        lda     _hgr_low,y        ; Update HGR pointer
        sta     ptr1
        lda     _hgr_hi,y
        sta     ptr1+1
.endif
        sty     _y0               ; Store Y

:
.ifdef INLINE_PLOT
        asl     xbita+1           ; Shift pixel mask
        bpl     :+
        inc     xdiv7a+1          ; Switch to next byte
        lda     #$01
        sta     xbita+1
:
.endif
        ldx     _x0               ; Check if end
        inx
        cpx     _x1
        bne     next_x            ; Carry clear if looping
        rts

slope_y:
        lda     _deltax           ; Compute slope err
        ldx     _deltay
        jsr     get_slope_err
        sta     slope_err

next_y:
.ifdef INLINE_PLOT
xdiv7b:
        ldy     #$FF
xbitb:
        lda     #$FF
        ora     (ptr1),y
        sta     (ptr1),y
.else
        ldx     _x0
        ldy     _y0
        jsr     _plot_dot
.endif
        lda     error             ; Update error
        clc                       ; Carry may not be clear (if Y decrements)
        adc     slope_err
        sta     error
        bcc     :+
.ifdef INLINE_PLOT
        asl     xbitb+1           ; Shift pixel mask
        bpl     :+
        inc     xdiv7b+1          ; Switch to next byte
        lda     #$01
        sta     xbitb+1           ; (Don't bother with _x0 as we don't test it)
.else
        inc     _x0
.endif
:
        ldy     _y0
move_y2:
        NOP                   ; patched with iny/dey
.ifdef INLINE_PLOT
        lda     _hgr_low,y
        sta     ptr1
        lda     _hgr_hi,y
        sta     ptr1+1
.endif
        sty     _y0
        cpy     _y1
        bne     next_y
        rts
.endproc

.bss
; _x0: .res 1
; _x1: .res 1
; _y0: .res 1
; _y1: .res 1
; error: .res 1
; slope_err: .res 1
_deltax: .res 1
_deltay: .res 1
