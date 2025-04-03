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

        .export     _crash_lines_scale, _draw_crash_lines

        .export     ox, oy, dx, dy ; Used by draw_score
        .export     _set_color_white

        .import     ___randomize, _rand
        .import     pushax, popax, pusha, popa

        .importzp   tmp3, tmp4

        .include    "sprite.inc"
        .include    "puck_coords.inc"
        .include    "constants.inc"
        .include    "hgr_applesoft.inc"

.proc rand_crash
        jsr     _rand
        lsr                   ; Divide to 32
        lsr
        lsr
extra_lsr:
        lsr
        clc
        rts
.endproc

.proc _crash_lines_scale
        ldx     #$4A  ; LSR
        bcc     :+
        ldx     #$EA  ; NOP
:       stx     rand_crash::extra_lsr
        rts
.endproc

.proc _draw_crash_lines
        jsr     ___randomize
        jsr     _set_color_white

        ; Set recursion level
        lda     #3
        sta     crash_lines_recursion

        ; Set origin
        lda     puck_data+SPRITE_DATA::WIDTH
        lsr
        sta     tmp4
        lda     puck_gx
        clc
        adc     tmp4
        bcc     :+
        lda     #255
:       tax
        lda     puck_gy
        jmp     _draw_lines_pair
.endproc

.proc _set_color_white
        bit     $C082
        ldx     #WHITE
        jsr     SETHCOL
        bit     $C080
        rts
.endproc

; origin coords in XA
; left/right in Y
.proc _draw_line
        sty     tmp3
        ; Origin in XA, save them
        stx     ox
        sta     oy
        ldy     #0
        bit     $C082
        jsr     HPOSN
        bit     $C080

        ; Get random height
        jsr     rand_crash
        clc
        adc     #10           ; Minimum 10
        sta     tmp4

        ; Go up
        lda     oy
        sec
        sbc     tmp4
        bcs     :+
        lda     #0
:       sta     dy
        tay

        ; Get random X delta
        jsr     rand_crash
        sta     tmp4

        lda     ox
        bit     tmp3
        bmi     do_left

do_right:
        sec
        sbc     tmp4
        bcs     :+
        lda     #0
        beq     :+

do_left:
        clc
        adc     tmp4
        bcc     :+
        lda     #255

:       sta     dx

        ldx     #0
        bit     $C082
        jsr     HLIN
        bit     $C080

        rts
.endproc

; Origin in XA
.proc _draw_lines_pair
        ; Save origin
        jsr     pushax

        ; Draw to the left
        ldy     #$FF
        jsr     _draw_line
        ; End of new line now in dx/dy

        ; Decrement recursion level
        lda     crash_lines_recursion
        sec
        sbc     #1
        sta     crash_lines_recursion

        beq     :+
        jsr     pusha             ; backup recursion level

        ; Draw a pair of lines at the end of our left line
        ldx     dx
        lda     dy
        jsr     _draw_lines_pair
        jsr     popa
        sta     crash_lines_recursion
:
        ; Get origin back
        jsr     popax

        ; Draw line to the right
        ldy     #$01
        jsr     _draw_line
        lda     crash_lines_recursion
        beq     :+

        jsr     pusha             ; backup recursion level

        ; Draw a pair of lines at the end of our right line
        ldx     dx
        lda     dy
        jsr     _draw_lines_pair
        jsr     popa
        sta     crash_lines_recursion
:
        rts
.endproc

.bss

ox:                    .res 1
oy:                    .res 1
dx:                    .res 1
dy:                    .res 1
crash_lines_recursion: .res 1
