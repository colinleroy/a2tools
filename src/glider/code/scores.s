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

        .export   _hi_scores_screen
        .export   _load_and_show_high_scores

        .import   large_tmp_buf
        .import   _your_name_str
        .import   _high_scores_str
        .import   _read_string
        .import   _cputsxy
        .import   _wait_for_input
        .import   _str_input

        .import   bcd_input, _cutoa

        .import   _high_scores_io, cur_score

        .import   pushax, pusha, _gotoxy, _clrscr

        .include  "apple2.inc"
        .include  "fcntl.inc"
        .include  "scores.inc"

.proc _hi_scores_screen
        lda     #<O_RDONLY
        jsr     _high_scores_io

        jsr     _find_score_spot
        bcc     :+
        ; We don't have a high score, but still, show them
        jmp     _show_high_scores
:
        ; We have a spot at Y in the scores table. First backup that spot
        sty     score_spot

        ; Now shift everything at that spot and after, excluding
        ; the last line, by one line
        ldy     #SCORE_TABLE_SIZE-.sizeof(SCORE_LINE)
        ldx     #SCORE_TABLE_SIZE

:       dex
        dey
        lda     large_tmp_buf,y
        sta     large_tmp_buf,x
        cpy     score_spot
        bne     :-

        jsr     _clrscr

        ldx     #0
        lda     #3
        jsr     pushax
        lda     #<_your_name_str
        ldx     #>_your_name_str
        jsr     _cputsxy

        lda     #(MAX_NAME_LEN-1)
        jsr     _read_string

        ; Insert score in table
        jsr     _set_high_score

        ; Save the scores file
        lda     #(O_WRONLY|O_CREAT)
        jsr     _high_scores_io

        jmp     _show_high_scores
.endproc

.proc _find_score_spot
        ldy     #$00

:       ldx     large_tmp_buf,y
        iny
        lda     large_tmp_buf,y
        
        cpx     cur_score+1
        bcc     found
        bne     check_next_score
        cmp     cur_score
        bcc     found
        beq     found

check_next_score:
        tya
        clc
        ; Point to end of line
        adc     #.sizeof(SCORE_LINE::NAME)+1
        tay
        cpy     #SCORE_TABLE_SIZE
        bcc     :-

        ; Found no score lower than ours.
        ; Return with carry set
        rts
found:
        dey                       ; Put Y back to start of record
        clc
        rts
.endproc

.proc _set_high_score
        ldy     score_spot        ; Update score at spot (Y points
        lda     cur_score+1       ; to start of line in table
        sta     large_tmp_buf,y
        iny
        lda     cur_score
        sta     large_tmp_buf,y

        tya                       ; Now point Y to end of line in
        clc                       ; the table
        adc     #.sizeof(SCORE_LINE::NAME)
        tay

        ldx     #.sizeof(SCORE_LINE::NAME)-1
:       lda     _str_input,x      ; Read the full name from str input
        sta     large_tmp_buf,y   ; And set it in the table
        dey
        dex
        bpl     :-
        rts
.endproc

.proc _load_and_show_high_scores
        lda     #O_RDONLY
        jsr     _high_scores_io
.endproc

.proc _show_high_scores
        jsr     _clrscr

        ldx     #5
        lda     #3
        jsr     pushax
        lda     #<_high_scores_str
        ldx     #>_high_scores_str
        jsr     _cputsxy

        ldy     #5
        sty     y_coord

        lda     #$00
        sta     score_spot        ; Reuse for listing

next_score:
        inc     y_coord           ; Compute line coord

        ldy     score_spot
        lda     large_tmp_buf,y
        sta     bcd_input+1
        iny
        lda     large_tmp_buf,y
        sta     bcd_input
        ora     bcd_input+1
        beq     out             ; Stop at 0

        lda     #8
        jsr     pusha
        lda     y_coord
        jsr     _gotoxy
        lda     bcd_input
        ldx     bcd_input+1
        jsr     _cutoa

        ldx     #15
        lda     y_coord
        jsr     pushax
        lda     #<(large_tmp_buf+2)
        ldx     #>(large_tmp_buf+2)
        clc
        adc     score_spot
        bcc     :+
        inx
:       jsr     _cputsxy

        lda     score_spot
        clc
        adc     #.sizeof(SCORE_LINE)
        sta     score_spot
        cmp     #(NUM_SCORES*.sizeof(SCORE_LINE))
        bne     next_score
out:
        jmp     _wait_for_input
.endproc

        .bss

score_spot:     .res 1
y_coord:        .res 1
