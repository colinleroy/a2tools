        .export   _hi_scores_screen
        .import   _scores_table

        .import   _your_name_str
        .import   _read_string
        .import   _print_string
        .import   _clear_hgr_screen
        .import   _str_input

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
        rts                       ; We don't have a high score
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

        ldx     #3
        ldy     #30
        jsr     _print_string

        ; X and Y still valid
        lda     #MAX_NAME_LEN
        jsr     _read_string

        jsr     _set_high_score
        lda     #(O_WRONLY|O_CREAT)
        jmp     _high_scores_io
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

        .bss

score_spot:     .res 1
