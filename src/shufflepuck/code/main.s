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

        .export   _main
        .export   serving, my_score, their_score, turn
        .export   _check_keyboard, _last_key
        .export   _draw_opponent
        .export   won_tournament
        .export   game_cancelled
        .export   _init_puck_position, _set_puck_position

        .import   _hgr_force_mono40, vbl_ready
        .import   _mouse_setbarbox, _mouse_setplaybox

        .import   _choose_opponent, _add_hall_of_fame

        .import   puck_x, puck_y, puck_dx, puck_dy, bounces
        .import   _init_precise_x, _init_precise_y, _transform_puck_coords

        .import   their_pusher_x, their_pusher_y
        .import   their_pusher_dx, their_pusher_dy

        .import   _puck_reinit_my_order, _puck_reinit_their_order
        .import   puck_in_front_of_me, prev_puck_in_front_of_me
        .import   puck_in_front_of_them, prev_puck_in_front_of_them
        .import   _draw_screen, _clear_screen, _draw_scores
        .import   _move_puck, _puck_check_my_hit, _puck_check_their_hit
        .import   _move_my_pusher, _move_their_pusher, _round_end
        .import   their_hit_check_via_serial

        .import   __OPPONENT_START__

        .import   _print_load_error
        .import   _backup_table, _restore_table
        .import   _load_opponent
        .import   _restore_bar
        .import   _restore_bar_code
        .import   _restore_barsnd
        .import   _cache_working
        .import   _play_bar
        .import   _bar_update_champion

        .import   hz

        .import   _mouse_wait_vbl

        .import   _platform_msleep
        .import   _build_hgr_tables
        .import   _init_caches

        .import   _text_mono40, _home, _exit

        .import   ___randomize

        .include  "apple2.inc"
        .include  "sprite.inc"
        .include  "puck_coords.inc"
        .include  "my_pusher_coords.inc"
        .include  "my_pusher0.gen.inc"
        .include  "constants.inc"
        .include  "scores.inc"
        .include  "opponent_file.inc"

.segment "CODE"

; How much do we have left?
; .res 1650

.proc _main
        jsr     _init_caches
        jsr     ___randomize
        jsr     _build_hgr_tables

        jmp     _real_main
.endproc

.proc _draw_opponent_parts
        ldx     #(7/7)
        ldy     #39
        jsr     __OPPONENT_START__+OPPONENT::NAME
        ; Fallthrough
.endproc
.proc _draw_opponent
        ; Draw its sprite
        ldx     #(98/7)
        ldy     #76
        jmp     __OPPONENT_START__+OPPONENT::SPRITE
.endproc

.segment "LOWCODE"

.proc _real_main
        lda     _cache_working
        bne     play_barsnd       ; First arrival in _real_main,
                                  ; no need to restore sound, it's the
                                  ; last loaded thing.
new_opponent:
        lda     in_tournament     ; Are we in a tournament?
        beq     to_bar

        inc     opponent          ; Yes, go directly to the next one
        lda     opponent
        cmp     #NUM_OPPONENTS
        bne     new_game          ; Continue tournament

        lda     #1                ; We won the tournament
        sta     won_tournament
        lda     #0
        sta     in_tournament

to_bar:
        ; Entering the bar, clear screen, reload the intro sound, play it
        jsr     _home
        jsr     _text_mono40
        jsr     _restore_barsnd
        bcs     :+

play_barsnd:
        ldy     #0
        jsr     _play_bar

        ; Reload the bar image and code
:       jsr     _restore_bar
        bcc     :+
        jmp     _print_load_error

:       jsr     _restore_bar_code
        bcc     :+
        jmp     _print_load_error

:       ; Display the current champion
        jsr     _bar_update_champion

        ; Reinit HGR
        jsr     _hgr_force_mono40

        ; Update mouse boundaries for bar
        jsr     _mouse_setbarbox

        ; Did we win the tournament?
        lda     won_tournament
        beq     :+

        ; Yes, so go add player name's to hall of fame
        jsr     _add_hall_of_fame

        ; And reset the flag
        lda     #0
        sta     won_tournament

        ; Wait for player to choose an opponent
:       jsr     _choose_opponent
        bpl     store_opponent    ; > 0 = opponent number or ESC,
        lda     #1                ; < 0 = Start a tournament
        sta     in_tournament
        lda     #0                ; Start with opponent 0

store_opponent:
        sta     opponent
        cmp     #CH_ESC
        bne     new_game
        jmp     _exit

new_game:
        ; Bind mouse to table
        jsr     _mouse_setplaybox

        ; Reset variables
        lda     #$00
        sta     turn
        sta     my_score
        sta     their_score
        sta     game_cancelled
        sta     their_hit_check_via_serial

        ; Load the opponent file
        lda     opponent
        jsr     _load_opponent
        bcs     to_bar            ; Don't crash if we can't load the opponent

        ; Restore the table image,
        jsr     _restore_table
        bcc     :+
        jmp     _print_load_error

:       ; Draw the new opponent on it,
        jsr     _draw_opponent_parts
        ; And backup that.
        jsr     _backup_table
        ; If we can't back it up, update scores
        bcs     draw_scores

new_point:
        ; Check for end of game
        lda     #MAX_SCORE
        cmp     my_score
        beq     my_win
        cmp     their_score
        bne     cont_game

their_win:
        lda     #0                ; We lost so we also lost the tournament
        sta     in_tournament
my_win:
        lda     #<500
        ldx     #>500
        jsr     _platform_msleep
        jsr     __OPPONENT_START__+OPPONENT::END_GAME

next_or_new_opponent:
        jsr     _clear_screen
        jmp     new_opponent

clear_and_go_bar:
        lda     #0
        sta     in_tournament
        sta     won_tournament
        beq     next_or_new_opponent    ; Always branch

cont_game:
        ; Restore table from the backup
        jsr     _restore_table

        ; But if we can't, redraw the opponent
        lda     _cache_working
        bne     draw_scores
        jsr     _draw_opponent_parts

draw_scores:
        ; Draw scores
        jsr     _draw_scores

        ; Initialize coords
        jsr     _move_my_pusher

        ldy     #THEIR_PUSHER_INI_Y
        sty     their_pusher_y
        ldx     #THEIR_PUSHER_INI_X
        stx     their_pusher_x
        jsr     _move_their_pusher

        jsr     _init_puck_position

        ; Stop the puck, set serving flag
        ldx     #$00
        stx     puck_dx
        stx     puck_dy
        inx
        stx     serving

        ; Reset sprite stack
        jsr     _puck_reinit_my_order
        lda     puck_in_front_of_me
        sta     prev_puck_in_front_of_me
        jsr     _puck_reinit_their_order
        lda     puck_in_front_of_them
        sta     prev_puck_in_front_of_them

        ; Prepare the frame dropped for 60Hz machines
        lda     #6
        sta     frame_counter

game_loop:
        ; the WAI of the poor
        ; because I don't understand how WAI works
        ; and I want to keep things 6502-ok
        jsr     _mouse_wait_vbl

loop_start:
        ; Drop 1 frame out of 6 at 60Hz
        ; otherwise the game is harder
        lda     hz
        cmp     #60
        bne     draw_start
        dec     frame_counter
        bne     draw_start
        lda     #6
        sta     frame_counter
        bne     game_loop

draw_start:
        ; First thing is drawing the screen so we don't flicker
        jsr     _draw_screen

draw_done:
        ; If the puck is moving, service is done
        lda     puck_dy
        beq     :+
        lda     #$00
        sta     serving

:       ; Update our pusher position,
        jsr     _move_my_pusher

        ; Let the opponent think about what to do,
        jsr     __OPPONENT_START__+OPPONENT::THINK_CB

        ; And move their pusher accordingly
        jsr     _move_their_pusher

        ; Check for pause (after THINK_CB so opponents can hook into keyboard)
        lda     _last_key
        cmp     #' '
        bne     :+
        jsr     pause

:       ; Check collision
        jsr     _puck_check_my_hit
        jsr     _puck_check_their_hit

        ; Update the puck's position,
        jsr     _move_puck
        jsr     __OPPONENT_START__+OPPONENT::HIT_CB
        ; and if carry is set, it reached the end of the table.
        bcs     reset_point

        lda     game_cancelled
        bne     abort_game

        ; Check for keyboard input
        jsr     _check_keyboard
        bcc     game_loop

        ; Keyboard hit, is it escape?
        cmp     #CH_ESC
        bne     :+
abort_game:
        jmp     clear_and_go_bar
:
.ifdef CHEAT 
        cmp     #'w'
        bne     game_loop
        lda     #15
        sta     my_score
        jmp     my_win
.else
        jmp     game_loop
.endif

reset_point:
        jsr     _round_end
        ; The point has been scored. Update the serving turn
        lda     turn
        eor     #$01
        sta     turn

        ; Reset bounces counter (for Nerual's DX calculation)
        lda     #$00
        sta     bounces

reset_point_cont:
        ; Move the puck to the serving point
        jsr     _puck_reinit_my_order
        jsr     _puck_reinit_their_order

        jsr     get_puck_return_x_speed
        stx     puck_dx
        jsr     get_puck_return_y_speed
        stx     puck_dy

update_screen:
        ; Redraw the full screen, without checking for collisions
        jsr     _mouse_wait_vbl
        jsr     _draw_screen
        jsr     _move_my_pusher
        jsr     _move_their_pusher
        jsr     _move_puck

        ; Update the puck's required DX/DY to reach the serving point
        jsr     get_puck_return_y_speed
        bne     reset_point_cont

        jsr     get_puck_return_x_speed
        bne     reset_point_cont

        ; It's in place! Prepare for service
        lda     #0
        sta     their_pusher_dx
        sta     their_pusher_dy
        sta     puck_dx
        sta     puck_dy
        jsr     _clear_screen
        jmp     new_point
.endproc

.proc _init_puck_position
        ldx     #PUCK_INI_X
        ldy     turn
        lda     turn_puck_y,y
        tay
        ; Fallthrough to _set_puck_position
.endproc

.proc _set_puck_position
        stx     puck_x
        sty     puck_y
        txa
        jsr     _init_precise_x
        tya
        jsr     _init_precise_y

        ; Set correct graphics coords first
        jmp     _transform_puck_coords
.endproc

; Return with required delta in X
.proc get_puck_return_y_speed
        ldx     turn
        lda     turn_puck_y,x
        sta     puck_serve_y

        ldx     #4                    ; DY required to move forward
        lda     puck_serve_y          ; puck_serve_y - 3 >= puck_y => continue
        sec
        sbc     #3
        cmp     puck_y
        bcs     out

        ldx     #<-4                  ; DY required to move backwards
        clc                           ; puck_serve_y + 3 < puck_y => continue
        adc     #6
        cmp     puck_y
        bcc     out
        ldx     #0                    ; Puck's Y is good, stop moving it
out:    rts
.endproc

; Return with required delta in X
.proc get_puck_return_x_speed
        ldx     #4
        lda     #(PUCK_INI_X-4)       ; puck_ini_x - 3 >= puck_x => continue
        cmp     puck_x
        bcs     out

        ldx     #<-4
        lda     #(PUCK_INI_X+4)       ; puck_ini_x + 3 < puck_x => continue
        cmp     puck_x
        bcc     out
        ldx     #0
out:    rts
.endproc

.proc _check_keyboard
        lda     KBD
        bpl     no_kbd
        bit     KBDSTRB
        and     #$7F
        sta     _last_key
        sec
        rts
no_kbd:
        lda     #$00
        sta     _last_key
        clc
        rts
.endproc

.proc pause
:       jsr     _check_keyboard
        bcc     :-
        lda     #$00
        sta     _last_key
        rts
.endproc

.data

turn_puck_y:
        .byte   MY_PUCK_INI_Y
        .byte   THEIR_PUCK_INI_Y

.bss

turn:           .res 1
puck_serve_y:   .res 1
serving:        .res 1

my_score:       .res 1
their_score:    .res 1
opponent:       .res 1
in_tournament:  .res 1
won_tournament: .res 1
frame_counter:  .res 1
_last_key:      .res 1
game_cancelled: .res 1
