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

        .export     _draw_scores, _draw_score_update
        .export     _score_x_start

        .import     my_score, their_score
        .import     _set_color_white
        .import     ox, oy
        .import     _mouse_wait_vbl

        .import     hand_data
        .import     _setup_sprite_pointer_for_clear, _setup_sprite_pointer_for_draw
        .import     _clear_sprite, _draw_sprite, _skip_top_lines
        .import     _platform_msleep

        .importzp   tmp1

        .include    "sprite.inc"
        .include    "puck_coords.inc"
        .include    "constants.inc"
        .include    "hand.gen.inc"
        .include    "hgr_applesoft.inc"

.data

_score_x_start:
        .byte 56, 58, 60, 62, 55
        .byte 68, 70, 72, 74, 67
        .byte 80, 82, 84, 86, 79

.segment "CODE"

.proc clear_hand
        lda     #<hand_data
        ldx     #>hand_data
        jsr     _setup_sprite_pointer_for_clear
        jmp     _clear_sprite
.endproc

.proc get_hand_x
        ldy     cur_score_draw
        dey
        lda     _score_x_start,y
        rts
.endproc

; Y: the current point
; Return with carry clear if diagonal
.proc is_point_diagonal
        cpy     #4
        beq     diagonal
        cpy     #9
        beq     diagonal
        cpy     #14
        beq     diagonal

        sec
        rts
diagonal:
        clc
        rts
.endproc

THEIR_SCORE_Y_OFFSET = 14
HAND_TOP = 8

.proc get_hand_y
        lda     #(HAND_TOP+hand_HEIGHT-SCORE_VERTICAL_START_Y)
        ldy     cur_score_draw
        dey
        jsr     is_point_diagonal
        bcs     :+
        lda     #(HAND_TOP+hand_HEIGHT-SCORE_DIAGONAL_START_Y)
        sec
:       sbc     hand_y_offset
        rts
.endproc

; X= X direction (00 or 02)
; Y= Y direction (FF or 01)
; A= end of animation
.proc animate_hand
        sta     stop_at+1
        sty     update_y+1
        stx     delta_x+1

next_frame:
        jsr     _mouse_wait_vbl
        jsr     clear_hand
        lda     cur_score_draw_lines_skip

        lda     start_x
        sec
        sbc     #hand_WIDTH
        sta     hand_data+SPRITE_DATA::X_COORD
        lda     #HAND_TOP+1                ; Bottom of lamp
        sta     hand_data+SPRITE_DATA::Y_COORD

        ; Should we plot a dot?
        lda     hand_pen_down
        beq     :+

draw_point:
        jsr     _set_color_white
        ldx     start_x
        lda     #(HAND_TOP+hand_HEIGHT)
        sec
        sbc     cur_score_draw_lines_skip
        jsr     plot_dot

:       ; reset total number of bytes
        lda     #hand_BYTES-1
        sta     hand_data+SPRITE_DATA::BYTES

        lda     #<hand_data
        ldx     #>hand_data
        jsr     _setup_sprite_pointer_for_draw

        lda     cur_score_draw_lines_skip
        jsr     _skip_top_lines

tmp_draw_hand:
        jsr     _draw_sprite

sleep_delay:
        lda     #$FF
        ldx     #0
        jsr     _platform_msleep

        lda     start_x
        clc
delta_x:
        adc     #$FF
        sta     start_x

        lda     cur_score_draw_lines_skip
        clc
update_y:
        adc     #$FF
        sta     cur_score_draw_lines_skip

stop_at:
        cmp     #$FF
        beq     out

        jmp     next_frame
out:    rts

.endproc

.proc _draw_score_update
        ldx     my_score
        ldy     #0

        bcc     :+
        ldx     their_score
        ldy     #THEIR_SCORE_Y_OFFSET

:       stx     cur_score_draw
        sty     hand_y_offset
        jsr     get_hand_y
        sta     end_score_draw_lines_skip

        jsr     get_hand_x
        sta     start_x

        lda     #hand_HEIGHT-1
        sta     cur_score_draw_lines_skip

        ; Move start_x to the left so the hand moves diagonally
        sec
        sbc     end_score_draw_lines_skip
        sta     tmp1

        lda     start_x
        sec
        sbc     tmp1
        sta     start_x

        ; Get hand down
        lda     #10
        sta     animate_hand::sleep_delay+1
        lda     end_score_draw_lines_skip
        ldy     #$FF              ; Go down
        ldx     #0
        stx     hand_pen_down
        inx
        jsr     animate_hand

        ; ; Draw current point
        lda     #(25*7/5)
        sta     animate_hand::sleep_delay+1
        ldy     cur_score_draw
        dey
        ldx     #2                ; DX for diagonal
        lda     cur_score_draw_lines_skip
        clc
        adc     #(SCORE_DIAGONAL_LINE_LEN)

        jsr     is_point_diagonal
        bcc     :+
        lda     #(25*7/7)
        sta     animate_hand::sleep_delay+1
        ldx     #0                ; DX for vertical
        lda     cur_score_draw_lines_skip
        clc
        adc     #(SCORE_VERTICAL_LINE_LEN)

:       ldy     #$01              ; Go up
        sty     hand_pen_down
        jsr     animate_hand

        ; Get hand up
        lda     #10
        sta     animate_hand::sleep_delay+1
        lda     #hand_HEIGHT
        ldy     #$01
        ldx     #0
        stx     hand_pen_down
        dex
        jsr     animate_hand

        ; Final clear
        jmp     clear_hand
.endproc

.segment "LOWCODE"

; X,A: x-y coordinates
.proc plot_dot
        ldy     #0
        bit     $C082
        jsr     HPLOT
        bit     $C080
        rts
.endproc

; We use plot instead of HLINE because it
; renders more cleanly
.proc draw_score_line
:       ldx     ox
        lda     oy
        jsr     plot_dot

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
        sta     next_line+1
        stx     y_offset+1

        lda     #0
        sta     cur_line

next_line:
        lda     #$FF
        cmp     cur_line
        beq     out

        ldy     cur_line
        lda     _score_x_start,y
        sta     ox

        jsr     is_point_diagonal
        bcc     diagonal
vertical:
        ldx     #SCORE_VERTICAL_LINE_LEN
        ldy     #SCORE_VERTICAL_X_OFFSET
        lda     #SCORE_VERTICAL_START_Y
        clc
        jmp     y_offset
diagonal:
        ldx     #SCORE_DIAGONAL_LINE_LEN
        ldy     #SCORE_DIAGONAL_X_OFFSET
        lda     #SCORE_DIAGONAL_START_Y

y_offset:
        adc     #$FF
        sta     oy
        stx     len
        sty     x_offset

        jsr     draw_score_line
        inc     cur_line
        bne     next_line

out:    rts
.endproc

.proc _draw_scores
        jsr     _set_color_white
        ldx     #0
        lda     my_score
        jsr     draw_one_player_scores

        ldx     #THEIR_SCORE_Y_OFFSET
        lda     their_score
        jmp     draw_one_player_scores
.endproc

.bss
cur_line: .res 1
len:      .res 1
x_offset: .res 1
cur_score_draw: .res 1
end_score_draw_lines_skip: .res 1
cur_score_draw_lines_skip: .res 1
start_x:  .res 1
hand_y_offset: .res 1
hand_pen_down: .res 1
