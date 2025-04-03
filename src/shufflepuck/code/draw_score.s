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

        .export     _draw_scores

        .import     my_score, their_score
        .import     _set_color_white
        .import     ox, oy, dx, dy

        .importzp   tmp3, tmp4

        .include    "sprite.inc"
        .include    "puck_coords.inc"
        .include    "constants.inc"
        .include    "hgr_applesoft.inc"

.data

score_x_start:
        .byte 56, 58, 60, 62, 55
        .byte 68, 70, 72, 74, 67
        .byte 80, 82, 84, 86, 79

VERTICAL_LINE_LEN = 7
DIAGONAL_LINE_LEN = 5
VERTICAL_X_OFFSET = 0
DIAGONAL_X_OFFSET = 2
VERTICAL_START_Y  = 23
DIAGONAL_START_Y  = 22

.code

; We use plot instead of HLINE because it
; renders more cleanly
.proc draw_score_line
:       ldx     ox
        lda     oy
        ldy     #0
        bit     $C082
        jsr     HPLOT
        bit     $C080

        dec     oy
        lda     ox
        clc
        adc     x_offset
        sta     ox

        dec     len
        bne     :-
        rts
.endproc

.proc draw_one_player_scores
        sta     player_score+1
        stx     y_offset+1

        lda     #0
        sta     cur_line

player_score:
:       lda     #$FF
        cmp     cur_line
        beq     :+

        ldy     cur_line
        lda     score_x_start,y
        sta     ox

        cpy     #4
        beq     diagonal_line
        cpy     #9
        beq     diagonal_line
        cpy     #14
        beq     diagonal_line

vertical_line:
        ldx     #VERTICAL_LINE_LEN
        ldy     #VERTICAL_X_OFFSET
        lda     #VERTICAL_START_Y
        bne     add_opponent_y

diagonal_line:
        ldx     #DIAGONAL_LINE_LEN
        ldy     #DIAGONAL_X_OFFSET
        lda     #DIAGONAL_START_Y

add_opponent_y:
        clc
y_offset:
        adc     #$FF
        sta     oy
        stx     len
        sty     x_offset

        jsr     draw_score_line
        inc     cur_line
        bne     :-

:       rts
.endproc

.proc _draw_scores
        jsr     _set_color_white
        ldx     #0
        lda     my_score
        jsr     draw_one_player_scores

        ldx     #14
        lda     their_score
        jmp     draw_one_player_scores
.endproc

.bss
cur_line: .res 1
len:      .res 1
x_offset: .res 1
