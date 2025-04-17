        .export     _choose_opponent, _add_hall_of_fame
        .export     _print_champion, _set_champion

        .import     _scores_buffer

        .import     _mouse_wait_vbl, _mouse_check_button
        .import     _clear_sprite, _draw_sprite
        .import     _setup_sprite_pointer_for_clear
        .import     _setup_sprite_pointer_for_draw

        .import     _load_hall_of_fame, _restore_bar
        .import     _load_scores, _save_scores

        .import     my_score, their_score

        .import     puck_x, mouse_x
        .import     puck_y, mouse_y
        .import     _puck_select
        .import     _check_keyboard

        .import     _print_string, _read_string, _str_input
        .import     _print_char, _print_number, bcd_input

        .import     _init_text, _init_hgr
        .import     _cputs, _cgetc, _clrscr, _strlen
        .import     _exit, pushax, _memmove, _strcpy, _bzero

        .import     __OPPONENT_START__

        .importzp   ptr2, tmp1

        .include    "apple2.inc"
        .include    "sprite.inc"
        .include    "font.gen.inc"
        .include    "pointer.gen.inc"
        .include    "pointer_coords.inc"
        .include    "constants.inc"
        .include    "scores.inc"

.segment "bar_code"

BOXES_START = *
opponents_boxes:                      ; X, W, Y, H, opponent number
        .byte 39,  17, 104, 26, 0     ; Skip
        .byte 196, 52, 172, 18, 1     ; Visine
        .byte 181, 54, 97,  48, 2     ; Vinnie
        .byte 15,  32, 68,  32, 3     ; Lexan
        .byte 73,  38, 94,  36, 4     ; Eneg
        .byte 7,   28, 21,  39, 5     ; Nerual
        .byte 129, 24, 58,  41, 6     ; Bejin
        .byte 245, 10, 77,  51, 7     ; Biff
        .byte 58 , 26, 23,  32, 8     ; DC3 (not in tournament)
        .byte 83,  87, 12,  34, $FF   ; Tournament
        .byte 231, 21, 13,  21, CH_ESC; Exit
NUM_BOXES = (* - BOXES_START)/5

CHAMPION_LEN    = 12
CHAMPION_LEFT   = 88
CHAMPION_BOTTOM = 34

MENU_BOTTOM    = CHAMPION_BOTTOM + 3 + 5
MENU_TOP       = CHAMPION_BOTTOM - 5

empty_str:    .asciiz "            "
champion_str: .asciiz "      ?     "
congrats_str: .asciiz "CONGRATS! PLEASE ENTER YOUR NAME:"
fight_str:    .asciiz "TOURNAMENT"
view_str:     .asciiz "VIEW ROSTER"

.proc _set_champion
        lda     #<champion_str
        ldx     #>champion_str
        jsr     pushax
        lda     #<(_scores_buffer+2)
        ldx     #>(_scores_buffer+2)
        jmp     _strcpy
.endproc

.proc _print_champion
        jsr     clear_champion
        lda     #<champion_str
        ldx     #>champion_str
        jsr     pushax            ; Push for print_string

        jsr     _strlen           ; Center string
        sta     tmp1
        lda     #CHAMPION_LEN
        sec
        sbc     tmp1
        lsr
        clc
        adc     #(CHAMPION_LEFT/7)
        tax
        ldy     #CHAMPION_BOTTOM
        jmp     _print_string
.endproc

.proc clear_champion
        lda     #<empty_str
        ldx     #>empty_str
        jsr     pushax
        jsr     pushax
        ldx     #(CHAMPION_LEFT/7)
        ldy     #MENU_BOTTOM
        jsr     _print_string
        ldx     #(CHAMPION_LEFT/7)
        ldy     #CHAMPION_BOTTOM
        jmp     _print_string
.endproc

.proc print_menu
        jsr     clear_champion
        lda     #<fight_str
        ldx     #>fight_str
        jsr     pushax
        ldx     #(CHAMPION_LEFT/7)
        ldy     #CHAMPION_BOTTOM
        jsr     _print_string
        lda     #<view_str
        ldx     #>view_str
        jsr     pushax
        ldx     #(CHAMPION_LEFT/7)
        ldy     #MENU_BOTTOM
        jmp     _print_string
.endproc

.proc _choose_opponent
        ; Only reprint Champion board when needed
        jsr     clear_pointer
        lda     in_menu
        beq     show_champion
        jsr     print_menu
        jmp     wait_input
show_champion:
        jsr     _print_champion

wait_input:
        jsr     _mouse_wait_vbl
        lda     mouse_x
        sta     pointer_x
        lda     mouse_y
        sta     pointer_y
        jsr     clear_pointer
        jsr     draw_pointer

        jsr     _check_keyboard
        bcs     kbd
        jsr     _mouse_check_button
        bcc     wait_input

        ; Wait for button to be up
:       jsr     _mouse_check_button
        bcs     :-

        lda     in_menu
        beq     search_opponent

        jsr     check_menu_choice
        bcc     close_menu
        bmi     return_opponent   ; User chose TOURNAMENT

        jsr     show_roster       ; User chose ROSTER (and A=0)

close_menu:
        lda     #0
        sta     in_menu
        beq     _choose_opponent  ; Branch always

search_opponent:
        jsr     find_opponent
        bcc     wait_input

        ; Did the user click the Champion sign?
        bmi     open_menu

return_opponent:                  ; Return the opponent number
        pha                       ; or $FF to start tournament
        jsr     clear_pointer
        pla
        rts

kbd:
        cmp     #CH_ESC
        bne     wait_input
        jmp     _exit

open_menu:
        lda     #1
        sta     in_menu
        jmp     _choose_opponent

handle_menu_choice:
        brk
.endproc

.proc find_opponent
        ldy     #0
check_next_opponent:
        lda     opponents_boxes,y ; check left
        cmp     mouse_x
        iny
        bcs     skip_y
        adc     opponents_boxes,y ; check left+width
        cmp     mouse_x
        bcc     skip_y

        iny
        lda     opponents_boxes,y ; check top
        cmp     mouse_y
        iny
        bcs     skip_num_opponent
        adc     opponents_boxes,y ; check top+height
        cmp     mouse_y
        bcc     skip_num_opponent

        iny
        lda     opponents_boxes,y ; Opponent number
        sec
        rts

skip_y: iny
        iny
skip_num_opponent:
        iny
        iny
        cpy     #((NUM_BOXES*5))
        bne     check_next_opponent
        clc
        rts
.endproc

.proc draw_pointer
        lda     #<pointer_data
        ldx     #>pointer_data
        jsr     _setup_sprite_pointer_for_draw
        jmp     _draw_sprite
.endproc

.proc clear_pointer
        lda     #<pointer_data
        ldx     #>pointer_data
        jsr     _setup_sprite_pointer_for_clear
        jmp     _clear_sprite
.endproc

.proc _add_hall_of_fame
        lda     #1
        jmp     show_roster
.endproc

; Returns with carry set if a choice is made
; A=$FF if user clicked TOURNAMENT
; A=$00 if user clicked VIEW ROSTER
; Carry clear if user clicked outside of choices
.proc check_menu_choice
        lda     mouse_x
        cmp     #CHAMPION_LEFT
        bcc     out
        cmp     #(CHAMPION_LEFT+(CHAMPION_LEN*7))
        bcs     out
        lda     mouse_y
        cmp     #MENU_TOP
        bcc     out
        cmp     #CHAMPION_BOTTOM
        bcc     do_tournament
        cmp     #MENU_BOTTOM
        bcc     view_roster
out:    clc
        rts

do_tournament:
        lda     #$FF
        sec
        rts
view_roster:
        lda     #$00
        sec
        rts
.endproc

; Enter with A=1 to add a name to the roster
.proc show_roster
        pha                       ; Backup whether we should add new name
        jsr     _load_hall_of_fame; Load image
        pla

        beq     :+
        jsr     shift_score_out

        ; Set our score
        lda     my_score
        sta     _scores_buffer
        lda     their_score
        sta     _scores_buffer+1

        jsr     display_scores
        jsr     add_name

        lda     #<_scores_buffer
        ldx     #>_scores_buffer
        jsr     _save_scores
        jsr     _set_champion
        jmp     _restore_bar

:       jsr     display_scores
        jsr     _mouse_check_button
        bcs     out
        lda     KBD
        bpl     :-
        bit     KBDSTRB

out:    jmp     _restore_bar
.endproc

SCORE_LINE_X_LEFT  = 14/7
SCORE_LINE_X_SCORE = MAX_WINNER_LEN+SCORE_LINE_X_LEFT
SCORE_LINE_X_RIGHT = 154/7
SCORE_LINE_X_RIGHT_SCORE = MAX_WINNER_LEN+SCORE_LINE_X_RIGHT
SCORE_LINE_Y_TOP   = 48-(font_HEIGHT+2)

.proc add_name
        lda     #<congrats_str
        ldx     #>congrats_str
        jsr     pushax
        ldx     #SCORE_LINE_X_LEFT
        ldy     #SCORE_LINE_Y_TOP
        jsr     _print_string

        ldx     #SCORE_LINE_X_LEFT
        lda     #SCORE_LINE_Y_TOP
        clc
        adc     #font_HEIGHT+2
        tay
        lda     #MAX_WINNER_LEN-1
        jsr     _read_string

        lda     #<(_scores_buffer+2)
        ldx     #>(_scores_buffer+2)
        jsr     pushax
        lda     #<_str_input
        ldx     #>_str_input
        jmp     _strcpy
.endproc

; Keep lines 0-(max-1) of the score table at 1-max
.proc shift_score_out
        lda     #<(_scores_buffer+.sizeof(SCORE_LINE))
        ldx     #>(_scores_buffer+.sizeof(SCORE_LINE))
        jsr     pushax
        lda     #<_scores_buffer
        ldx     #>_scores_buffer
        jsr     pushax
        lda     #<(SCORE_TABLE_SIZE-.sizeof(SCORE_LINE))
        ldx     #>(SCORE_TABLE_SIZE-.sizeof(SCORE_LINE))
        jsr     _memmove

        lda     #<_scores_buffer
        ldx     #>_scores_buffer
        jsr     pushax
        lda     #<.sizeof(SCORE_LINE)
        ldx     #>.sizeof(SCORE_LINE)
        jsr     _bzero
.endproc

.proc display_scores
        ldy     #SCORE_LINE_Y_TOP
        sty     cur_y

        lda     #<_scores_buffer
        ldx     #>_scores_buffer
        sta     ptr2
        stx     ptr2+1

        lda     #$00
        sta     cur_print
next_score:
        ; get left coordinate
        ldx     #SCORE_LINE_X_LEFT
        stx     cur_x               ; Store X
        ldx     #SCORE_LINE_X_SCORE
        stx     cur_x_score

        cmp     #(MAX_WINNERS/2)
        bcc     :+                  ; Right column
        ldx     #SCORE_LINE_X_RIGHT
        stx     cur_x               ; Store X
        ldx     #SCORE_LINE_X_RIGHT_SCORE
        stx     cur_x_score

        cmp     #(MAX_WINNERS/2)    ; First on right col, reset Y
        bne     :+

        ldy     #SCORE_LINE_Y_TOP
        sty     cur_y

:       lda     cur_y               ; Update Y
        clc
        adc     #font_HEIGHT+2
        sta     cur_y

print_one_score:
        ; My score
        ldy     #$00
        sty     bcd_input+1
        lda     (ptr2),y
        beq     out               ; We're done
        ldx     cur_x_score
        ldy     cur_y
        jsr     _print_number
        ; Separator
        lda     #'/'
        ldx     cur_x_score
        inx
        inx
        jsr     _print_char
        ; Their score
        ldy     #$01
        lda     (ptr2),y
        ldy     cur_y
        jsr     _print_number

        ; Name
        lda     ptr2
        ldx     ptr2+1
        clc
        adc     #2
        bcc     :+
        inx
:       jsr     pushax
        ldx     cur_x
        ldy     cur_y
        jsr     _print_string

        lda     ptr2
        clc
        adc     #<.sizeof(SCORE_LINE)
        bcc     :+
        inc     ptr2+1
:       sta     ptr2

        inc     cur_print
        lda     cur_print
        cmp     #MAX_WINNERS
        bne     next_score
out:    rts
.endproc

in_menu:    .byte 0
cur_print:  .byte 0
cur_x:      .byte 0
cur_x_score:.byte 0
cur_y:      .byte 0
