        .export     _choose_opponent, _add_hall_of_fame
        .export     _bar_load_scores, _bar_update_champion

        .import     _mouse_wait_vbl, _mouse_check_button
        .import     _clear_sprite, _draw_sprite

        .import     _load_hall_of_fame, _restore_bar
        .import     _load_scores, _save_scores
        .import     _backup_bar_code

        .import     my_score, their_score

        .import     mouse_x, mouse_y
        .import     _check_keyboard, _read_mouse

        .import     _print_string, _read_string, _str_input
        .import     _print_char, _print_number

        .import     _big_draw_moving_barcode_dc31, _big_draw_moving_barcode_dc32, _big_draw_moving_barcode_dc33
        .import     _big_draw_moving_barcode_nerual1, _big_draw_moving_barcode_nerual2
        .import     _big_draw_moving_barcode_lexan1, _big_draw_moving_barcode_lexan2, _big_draw_moving_barcode_lexan3

        .import     _strlen, _rand, ___randomize
        .import     _quit, pushax, _memmove, _strcpy, _bzero

        .importzp   tmp1, _zp6

        .include    "apple2.inc"
        .include    "sprite.inc"
        .include    "font.gen.inc"
        .include    "pointer.gen.inc"
        .include    "pointer_coords.inc"
        .include    "constants.inc"
        .include    "scores.inc"

.segment "barcode"

BOXES_START = *
opponents_boxes:                        ; X, W, Y, H, opponent number, name pointer
        .byte 39,  17, 104, 26, 0      ,>name_a, <name_a       ; Skip
        .byte 196, 52, 172, 18, 1      ,>name_b, <name_b       ; Visine
        .byte 181, 54, 97,  48, 2      ,>name_c, <name_c       ; Vinnie
        .byte 15,  32, 68,  32, 3      ,>name_d, <name_d       ; Lexan
        .byte 73,  38, 94,  36, 4      ,>name_e, <name_e       ; Eneg
        .byte 7,   28, 21,  39, 5      ,>name_f, <name_f       ; Nerual
        .byte 129, 24, 58,  41, 6      ,>name_g, <name_g       ; Bejin
        .byte 245, 10, 77,  51, 7      ,>name_h, <name_h       ; Biff
        .byte 58 , 26, 23,  32, 8      ,>name_i, <name_i       ; DC3 (not in tournament)
        .byte 35 , 21, 11,  29, 'N'-'A',>name_n, <name_n       ; Network
        .byte 83,  87, 12,  34, $FF    ,>name_t, <name_t       ; Tournament
        .byte 231, 21, 13,  21, CH_ESC ,>name_q, <name_q       ; Exit
NUM_BOXES = (* - BOXES_START)/7

name_a:         .asciiz "      SKIP      "
name_b:         .asciiz "     VISINE     "
name_c:         .asciiz "     VINNIE     "
name_d:         .asciiz "     LEXAN      "
name_e:         .asciiz "      ENEG      "
name_f:         .asciiz "     NERUAL     "
name_g:         .asciiz "     BEJIN      "
name_h:         .asciiz "      BIFF      "
name_i:         .asciiz "      DC3       "
name_n:         .asciiz "  TWO PLAYERS   "
name_t:         .asciiz "   TOURNAMENT   "
name_q:         .asciiz "   QUIT GAME    "
name_blank:     .asciiz "                "

CHAMPION_LEN    = 12
CHAMPION_LEFT   = 88
CHAMPION_BOTTOM = 34

MENU_BOTTOM     = CHAMPION_BOTTOM + 3 + 5
MENU_TOP        = CHAMPION_BOTTOM - 5

HOVER_X         = 35/7
HOVER_Y         = 191

empty_str:      .asciiz "            "
champion_str:   .asciiz "    BIFF    "
congrats_str:   .asciiz "CONGRATS! PLEASE ENTER YOUR NAME:"
fight_str:      .asciiz "TOURNAMENT"
view_str:       .asciiz "VIEW ROSTER"

; Copy first string from scores buffer to the champion string
.proc set_champion
        lda     _scores_buffer+2  ; Don't copy if empty
        bne     :+
        rts

:       lda     #<champion_str
        ldx     #>champion_str
        jsr     pushax
        lda     #<(_scores_buffer+2)
        ldx     #>(_scores_buffer+2)
        jmp     _strcpy
.endproc

; Print the champion on the board
.proc print_champion
        jsr     clear_champion
        lda     #<champion_str
        ldx     #>champion_str
        jsr     pushax            ; Push for print_string

        jsr     _strlen           ; Center string: strlen of name
        sta     tmp1
        lda     #CHAMPION_LEN     ; max len minus strlen
        sec
        sbc     tmp1
        lsr                       ; divided by two
        clc
        adc     #(CHAMPION_LEFT/7); Add left X
        tax
        ldy     #CHAMPION_BOTTOM
        jmp     _print_string
.endproc

; Clears the Champion sign
.proc clear_champion
        lda     #<empty_str
        ldx     #>empty_str
        jsr     pushax            ; Push twice, once for each line
        jsr     pushax
        ldx     #(CHAMPION_LEFT/7); Clear line 1
        ldy     #MENU_BOTTOM
        jsr     _print_string
        ldx     #(CHAMPION_LEFT/7); Clear line 2
        ldy     #CHAMPION_BOTTOM
        jmp     _print_string
.endproc

; Prints the menu (TOURNAMENT / VIEW ROSTER) on the Champion sign
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

; Wait for user to click on an opponent (or press Escape)
.proc _choose_opponent
        ; Only reprint Champion board when needed: when entering first,
        ; and when the display changes
        jsr     clear_pointer
        lda     in_menu
        beq     show_champion
        jsr     print_menu
        jmp     wait_input
show_champion:
        jsr     print_champion

; Loop waiting for input
wait_input:
        jsr     _mouse_wait_vbl   ; Avoid flickering
        jsr     clear_pointer
        jsr     animate_dc3
        jsr     animate_nerual
        jsr     animate_lexan
        jsr     draw_pointer

        ; Move around random while we wait for click
        inc     RNDL
        bne     :+
        inc     RNDH

:       jsr     _read_mouse
        lda     mouse_x
        sta     pointer_x
        lda     mouse_y
        sta     pointer_y

        jsr     do_hover

        ; Check for key
        jsr     _check_keyboard
        bcs     kbd

        ; Or for mouse button
        jsr     _mouse_check_button
        beq     wait_input

        ; Wait for button to be up (otherwise the Champion sign flickers)
:       jsr     _mouse_check_button
        bne     :-

        ; Reinit randomizer each click
        jsr     ___randomize

        ; Is the champion menu open?
        lda     in_menu
        beq     search_opponent

        ; Yes: check the user choice (TOURNAMENT, ROSTER, or out of the menu)
        jsr     check_menu_choice
        bcc     close_menu        ; User clicked outside of menu
        bmi     return_opponent   ; User chose TOURNAMENT (and A=$FF)

        jsr     show_roster       ; User chose ROSTER (and A=0 so "add" flag
                                  ; is off). ). Show the roster,
close_menu:
        lda     #0                ; And close menu
        sta     in_menu
        beq     _choose_opponent  ; Branch always to start of function, in
                                  ; order to reprint Champion sign

search_opponent:
        jsr     find_opponent     ; Check if mouse in an opponent's hitbox
        bcc     wait_input        ; No, go wait for input again

        ; Did the user click the Champion sign?
        bmi     open_menu         ; Yes, open the menu

return_opponent:                  ; Return the opponent number
        pha                       ; or $FF to start tournament
        jsr     clear_pointer
        pla
        rts

kbd:
        cmp     #CH_ESC           ; Same value as EXIT sign?
        bne     wait_input        ; No, go wait for input again
        jmp     _quit             ; Yes, exit

open_menu:
        lda     #1                ; Flag menu open and jump back
        sta     in_menu           ; to start of function in order to reprint
        jmp     _choose_opponent  ; the Champion sign
.endproc

.proc find_opponent
        ldy     #0
check_next_opponent:
        lda     opponents_boxes,y ; check left
        cmp     mouse_x
        iny
        bcs     skip_y_check
        adc     opponents_boxes,y ; check left+width
        cmp     mouse_x
        bcc     skip_y_check

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

skip_y_check:                     ; X didn't match so skip Y,H
        iny
        iny

skip_num_opponent:                ; Y didn't match so skip opponent
        iny
        iny                       ; and name pointer
        iny
        iny                       ; and advance to next hitbox

        cpy     #((NUM_BOXES*7))
        bne     check_next_opponent
        clc                       ; All hitboxes checked, none matched
        rts
.endproc

.proc draw_pointer
        lda     #<pointer_data
        ldx     #>pointer_data
        jmp     _draw_sprite
.endproc

.proc clear_pointer
        lda     #<pointer_data
        ldx     #>pointer_data
        jmp     _clear_sprite
.endproc

.proc _add_hall_of_fame
        lda     #1                ; Set "add" flag and show roster
        jmp     show_roster
.endproc

; Returns with carry set if a choice is made
; A=$FF if user clicked TOURNAMENT
; A=$00 if user clicked VIEW ROSTER
; Carry clear if user clicked outside of choices
.proc check_menu_choice
        lda     mouse_x
        cmp     #CHAMPION_LEFT
        bcc     out               ; Check X is on the right of the Champion sign's left border
        cmp     #(CHAMPION_LEFT+(CHAMPION_LEN*7))
        bcs     out               ; and on the left of its right border

        lda     mouse_y
        cmp     #MENU_TOP
        bcc     out               ; Check Y is under the top
        cmp     #(CHAMPION_BOTTOM+2)
        bcc     do_tournament     ; and over the end of the TOURNAMENT line
        cmp     #(MENU_BOTTOM+2)
        bcc     view_roster       ; or over the end of the VIEW ROSTER LINE
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
        jsr     shift_score_out   ; We should add a name, so drop the last one

        lda     my_score          ; Set our score (could be hardcoded, it has to be 15)
        sta     _scores_buffer
        lda     their_score       ; Set Biff's score
        sta     _scores_buffer+1

        jsr     display_scores    ; Display the score
        jsr     add_name          ; Let user add their name

        lda     #<_scores_buffer
        ldx     #>_scores_buffer
        jsr     _save_scores      ; Save the file
        jsr     set_champion      ; Update the Champion sign

        ; Re-backup our code so that the scores array
        ; is updated in the backup
        jsr     _backup_bar_code

        ; And restore the bar image
        jmp     _restore_bar

:       jsr     display_scores      ; Don't add name, just list scores.
        jsr     _mouse_check_button ; and wait for button click
        bne     out
        lda     KBD                 ; or keypress
        bpl     :-
        bit     KBDSTRB

out:    jmp     _restore_bar        ; and restore bar image
.endproc

SCORE_LINE_X_LEFT  = 14/7
SCORE_LINE_X_SCORE = MAX_WINNER_LEN+SCORE_LINE_X_LEFT
SCORE_LINE_X_RIGHT = 154/7
SCORE_LINE_X_RIGHT_SCORE = MAX_WINNER_LEN+SCORE_LINE_X_RIGHT
SCORE_LINE_Y_TOP   = 48-(font_HEIGHT+2)

; Add player name to the scores array
.proc add_name
        lda     #<congrats_str      ; Show the congrats line
        ldx     #>congrats_str
        jsr     pushax
        ldx     #SCORE_LINE_X_LEFT
        ldy     #SCORE_LINE_Y_TOP
        jsr     _print_string

        ldx     #SCORE_LINE_X_LEFT  ; Go to the first score line
        lda     #SCORE_LINE_Y_TOP
        clc
        adc     #font_HEIGHT+2
        tay
        lda     #MAX_WINNER_LEN-1
        jsr     _read_string        ; Read the name from keyboard

        lda     #<(_scores_buffer+2); Copy it to the scores array first line,
        ldx     #>(_scores_buffer+2); Right after the two score bytes
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
        ldy     #SCORE_LINE_Y_TOP   ; Start at top
        sty     cur_y

        lda     #<_scores_buffer    ; Set the buffer walker
        ldx     #>_scores_buffer
        sta     _zp6
        stx     _zp6+1

        lda     #$00                ; Store the current index
        sta     cur_print
next_score:
        ldx     #SCORE_LINE_X_LEFT  ; Get left coordinate for the name
        stx     cur_x               ; Store it
        ldx     #SCORE_LINE_X_SCORE ; Get left coordinate for the score
        stx     cur_x_score         ; Store it

        cmp     #(MAX_WINNERS/2)    ; Check which column we should use (A = current index)
        bcc     :+
        ldx     #SCORE_LINE_X_RIGHT ; Right column, update left coordinates
        stx     cur_x
        ldx     #SCORE_LINE_X_RIGHT_SCORE
        stx     cur_x_score

        cmp     #(MAX_WINNERS/2)    ; First line on right column?
        bne     :+

        ldy     #SCORE_LINE_Y_TOP   ; Yes, reset Y
        sty     cur_y

:       lda     cur_y               ; Update Y
        clc
        adc     #font_HEIGHT+2
        sta     cur_y

print_one_score:
        ; My score
        ldy     #$00
        sty     tmp1
        lda     (_zp6),y
        beq     out               ; If winner score is 0, we're done
        ldx     cur_x_score
        ldy     cur_y
        jsr     _print_number     ; Print it

        lda     #'/'              ; Separator
        jsr     _print_char

        ldy     #$00
        sty     tmp1
        iny                       ; Biff's score
        lda     (_zp6),y
        ldy     cur_y
        jsr     _print_number

        lda     _zp6              ; Player name
        ldx     _zp6+1
        clc
        adc     #2                ; Skip the two score bytes
        bcc     :+
        inx
:       jsr     pushax
        ldx     cur_x
        ldy     cur_y
        jsr     _print_string     ; Print the name

        lda     _zp6              ; Push walker to next line
        clc
        adc     #<.sizeof(SCORE_LINE)
        bcc     :+
        inc     _zp6+1
:       sta     _zp6

        inc     cur_print         ; Increment index
        lda     cur_print
        cmp     #MAX_WINNERS
        bne     next_score
out:    rts
.endproc

; Load the scores array from disk if not already done
.proc _bar_load_scores
        lda     _scores_loaded
        bne     out_done
        lda     #<_scores_buffer
        ldx     #>_scores_buffer
        jsr     _load_scores
out_done:
        lda     #1                ; Scores loaded
        sta     _scores_loaded
        rts
.endproc

; Load scores if needed, and print Champion's name
.proc _bar_update_champion
        jsr     _bar_load_scores
        jsr     set_champion
        jmp     print_champion
.endproc

.proc animate_dc3
        ldx     #((63)/7)
        ldy     #28
        lda     dc3_animate
        cmp     #15
        beq     dc3_animate_1
        cmp     #31
        beq     dc3_animate_2
        cmp     #47
        beq     dc3_animate_3
        cmp     #63
        beq     dc3_animate_4
        jmp     dc3_inc_animate
dc3_animate_1:
        jsr     _big_draw_moving_barcode_dc31
        jmp     dc3_inc_animate
dc3_animate_2:
        jsr     _big_draw_moving_barcode_dc32
        jmp     dc3_inc_animate
dc3_animate_3:
        jsr     _big_draw_moving_barcode_dc31
        jmp     dc3_inc_animate
dc3_animate_4:
        jsr     _big_draw_moving_barcode_dc33
        lda     #$FF              ; Will go to 0
        sta     dc3_animate
dc3_inc_animate:
        inc     dc3_animate
        rts
.endproc

.proc animate_nerual
        jsr     _rand
        ldx     #((21)/7)
        ldy     #32
        cmp     #2
        bcc     open_eyes
        cmp     #6
        bcc     close_eyes
        rts
open_eyes:
        jmp     _big_draw_moving_barcode_nerual1
close_eyes:
        jmp     _big_draw_moving_barcode_nerual2
.endproc

.proc animate_lexan
        ldx     #((21)/7)
        ldy     #74
        lda     lex_animate
        beq     maybe_animate_lexan
        cmp     #9
        beq     lexan_animate_1
        cmp     #19
        beq     lexan_animate_2
        cmp     #29
        beq     lexan_animate_3
        cmp     #39
        beq     lexan_animate_4
        jmp     lexan_inc_animate
lexan_animate_1:
        jsr     _big_draw_moving_barcode_lexan1
        jmp     lexan_inc_animate
lexan_animate_2:
        jsr     _big_draw_moving_barcode_lexan2
        jmp     lexan_inc_animate
lexan_animate_3:
        jsr     _big_draw_moving_barcode_lexan1
        jmp     lexan_inc_animate
lexan_animate_4:
        jsr     _big_draw_moving_barcode_lexan3
        lda     #0
        sta     lex_animate
        rts
maybe_animate_lexan:
        jsr     _rand
        cmp     #2
        bcs     out
lexan_inc_animate:
        inc     lex_animate
out:    rts
.endproc

.proc   do_hover
        jsr     find_opponent
        bcc     clear

        iny
        lda     opponents_boxes,y
        tax
        iny
        lda     opponents_boxes,y
        jmp     print_hover

clear:  lda     #<name_blank
        ldx     #>name_blank

print_hover:
        jsr     pushax            ; Push for print_string
        ldx     #HOVER_X
        ldy     #HOVER_Y
        jmp     _print_string
.endproc

dc3_animate:.byte 0
lex_animate:.byte 0
in_menu:    .byte 0
cur_print:  .byte 0
cur_x:      .byte 0
cur_x_score:.byte 0
cur_y:      .byte 0
_scores_loaded: .res 1
_scores_buffer: .res SCORE_TABLE_SIZE
