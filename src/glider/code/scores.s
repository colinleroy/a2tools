        .export   _hi_scores_screen
        .export   _load_and_show_high_scores

        .import   _scores_table
        .import   _your_name_str
        .import   _high_scores_str
        .import   _read_string
        .import   _print_string, _print_number
        .import   _clear_hgr_screen, _wait_for_input
        .import   _str_input

        .import   bcd_input

        .import   _high_scores_io, cur_score

        .import   pushax

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
        lda     _scores_table,y
        sta     _scores_table,x
        cpy     score_spot
        bne     :-

        jsr     _clear_hgr_screen

        lda     #<_your_name_str
        ldx     #>_your_name_str
        jsr     pushax

        ldx     #0
        ldy     #30
        jsr     _print_string

        ; X and Y still valid
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

:       ldx     _scores_table,y
        iny
        lda     _scores_table,y
        
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
        sta     _scores_table,y
        iny
        lda     cur_score
        sta     _scores_table,y

        tya                       ; Now point Y to end of line in
        clc                       ; the table
        adc     #.sizeof(SCORE_LINE::NAME)
        tay

        ldx     #.sizeof(SCORE_LINE::NAME)-1
:       lda     _str_input,x      ; Read the full name from str input
        sta     _scores_table,y   ; And set it in the table
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
        jsr     _clear_hgr_screen

        lda     #<_high_scores_str
        ldx     #>_high_scores_str
        jsr     pushax

        ldx     #5
        ldy     #30
        jsr     _print_string

        ldy     #39
        sty     y_coord

        lda     #$00
        sta     score_spot        ; Reuse for listing

next_score:
        lda     y_coord           ; Compute line coord
        clc
        adc     #9
        sta     y_coord

        ldy     score_spot
        lda     _scores_table,y
        sta     bcd_input+1
        iny
        lda     _scores_table,y
        ora     bcd_input+1
        beq     out             ; Stop at 0

        lda     _scores_table,y
        ldx     #8
        ldy     y_coord
        jsr     _print_number

        lda     #<(_scores_table+2)
        ldx     #>(_scores_table+2)
        clc
        adc     score_spot
        bcc     :+
        inx
:       jsr     pushax

        ldx     #15
        ldy     y_coord
        jsr     _print_string

        lda     score_spot
        clc
        adc     #.sizeof(SCORE_LINE)
        sta     score_spot
        cmp     #(NUM_SCORES*.sizeof(SCORE_LINE))
        bne     next_score
out:
        rts
.endproc

        .bss

score_spot:     .res 1
y_coord:        .res 1
