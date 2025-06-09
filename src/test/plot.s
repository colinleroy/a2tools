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

        .export   _test, _plot_dot

        .import   _build_hgr_tables, _platform_msleep
        .import   _init_hgr, _cgetc

        .import   pushax, _bzero

        .export   _hgr_hi, _hgr_low, _hgr_bit
        .export   _mod7_table, _div7_table

        .importzp ptr1

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
        jsr     _plot_dot     ; Plot at middle

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

.proc hline
        lda     _x0
        cmp     _x1
        bcs     negdx
posdx:  ldy     #$EE          ; INC $nnnn
        bne     finish_x_params
negdx:  ldy     #$CE          ; DEC $nnnn
finish_x_params:
        sty     move_x

        lda     _x0
plot_next:
        tax
        ldy     _y0
        jsr     _plot_dot
move_x:
        BIT     _x0
        lda     _x0
        cmp     _x1
        bne     plot_next
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

        ldy     _y0           ; Setup deltay and stepy

.ifdef INLINE_PLOT
        lda     _hgr_low,y
        sta     ptr1
        lda     _hgr_hi,y
        sta     ptr1+1
.endif
        tya
        cmp     _y1
        bcs     negdy
posdy:
        sec
        sbc     _y1
        ldy     #$EE          ; INC $nnnn
        bne     finish_y_params
negdy:  bne     :+
        jmp     hline
:       lda     _y1
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

.ifdef INLINE_PLOT
        jmp     init_ptr
.endif
next_dot:
.ifdef INLINE_PLOT
        ldx     _x0           ; Plot a dot
        lda     _div7_table,x
        tay
        lda     _mod7_table,x
        tax
        lda     _hgr_bit,x
        ora     (ptr1),y
        sta     (ptr1),y
.else
        ldx     _x0
        ldy     _y0
        jsr     _plot_dot
.endif

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

check_deltay:                 ; err2 >= deltay?
        bcc     err2_gt_dy    ; Shortcut, test sign as deltay <= 0
        cmp     _deltay
        lda     _b_err2+1
        sbc     _deltay+1
        bcc     check_deltax
        clc
err2_gt_dy:
        lda     _b_err        ; Yes. err += deltay,
        adc     _deltay
        sta     _b_err
        bcs     stepx
        dec     _b_err+1

stepx:
        BIT     _x0           ; x0 += stepx (BIT patched with INC/DEC)

check_deltax:                 ; deltax >= err2?
        ldy     _b_err2+1     ; Shortcut: test sign as deltax>=0
        bmi     dx_gt_err2    ; err2 < 0 so yes
        bne     next_dot      ; err2 > 255 so no
        lda     _deltax       ; Test low byte
        cmp     _b_err2
        bcc     next_dot      ; No. Go do next dot
dx_gt_err2:
        lda     _b_err        ; Yes. err += deltax
        clc
        adc     _deltax
        sta     _b_err
        bcc     stepy
        inc     _b_err+1

stepy:
        BIT     _y0           ; y0 += stepy (patched with INC/DEC)
        .ifdef INLINE_PLOT
init_ptr:
        ldy     _y0
        lda     _hgr_low,y
        sta     ptr1
        lda     _hgr_hi,y
        sta     ptr1+1
        .endif

        jmp     next_dot
out:    rts
.endproc

.proc _plot_line_applesoft
        bit     $C082
        ldx     #3            ; WHITE
        jsr     $F6EC         ; SETHCOL

        ldx     ox
        lda     oy
        ldy     #0
        jsr     $F457         ; HPOSN

        lda     dx
        ldy     dy
        ldx     #0
        jsr     $F53A         ; HLIN
        bit     $C080
        rts
.endproc

.proc _test
        jsr    _build_hgr_tables
        jsr    _init_hgr

next_algo:   
        lda    algorithm
        asl
        tay
        lda    functions,y
        sta    replot_a+1
        sta    replot_b+1
        iny
        lda    functions,y
        sta    replot_a+2
        sta    replot_b+2

        lda    #0
        sta    ox
        lda    #0
        sta    oy
        lda    #252
        sta    dx
        lda    #190
        sta    dy
replot_a:
        jsr    $FFFF

        lda    ox
        clc
        adc    #7
        bcs    next
        sta    ox
        lda    dx
        sec
        sbc    #7
        sta    dx
        jmp    replot_a

next:
        lda    #252
        sta    ox
        lda    #5
        sta    oy
        lda    #0
        sta    dx
        lda    #185
        sta    dy
replot_b:
        jsr    $FFFF

        lda    oy
        clc
        adc    #5
        cmp    #186
        bcs    out
        sta    oy
        lda    dy
        sec
        sbc    #5
        sta    dy
        jmp    replot_b

out:    jsr    _cgetc
        lda    #<$2000
        ldx    #>$2000
        jsr    pushax
        jsr    _bzero
        inc    algorithm
        lda    algorithm
        cmp    #(NUM_FUNCTIONS)
        bne    :+
        lda    #0
        sta    algorithm
:       jmp    next_algo
rts
.endproc

.bss
dx: .res 1
dy: .res 1
ox: .res 1
oy: .res 1
mid_x: .res 1
mid_y: .res 1

_x0: .res 1
_x1: .res 1
_y0: .res 1
_y1: .res 1
_deltax: .res 1
_deltay: .res 2
_b_err:  .res 2
_b_err2: .res 2

algorithm: .res 1

_hgr_hi:                .res 192
_hgr_low:               .res 192
_hgr_bit:               .res 7
_div7_table:            .res 256
_mod7_table:            .res 256

.data
functions: 
           .addr _plot_line_bresenham
           .addr _plot_line_applesoft
           .addr _plot_line_recursive

NUM_FUNCTIONS = (*-functions) / 2
