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

        .export   _plot_algorithms

        .import   _plot_line_recursive
        .import   _plot_line_bresenham
        .import   _plot_line_bresenham2
        .import   _plot_line_applesoft

        .import   _build_hgr_tables
        .import   _init_hgr, _cgetc

        .import   pushax, _bzero

        .export   _hgr_hi, _hgr_low, _hgr_bit
        .export   _mod7_table, _div7_table

        .export   dx, dy, ox, oy
        
        .importzp ptr1, tmp1, tmp2

.data

functions: 
            .addr _plot_line_applesoft
            .addr _plot_line_bresenham
            .addr _plot_line_bresenham2
            .addr _plot_line_recursive

NUM_FUNCTIONS = (*-functions) / 2

.code

.proc clear_screen
        lda    #<$2000
        ldx    #>$2000
        jsr    pushax
        jmp    _bzero
.endproc

.proc _plot_algorithms
        jsr    _build_hgr_tables
        jsr    clear_screen
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
        jsr    clear_screen
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

algorithm: .res 1

_hgr_hi:                .res 192
_hgr_low:               .res 192
_hgr_bit:               .res 7
_div7_table:            .res 256
_mod7_table:            .res 256
