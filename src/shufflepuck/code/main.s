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
        .export   serving, my_score, their_score
        .export   _check_keyboard, _last_key
        .export   _draw_opponent
        .export   won_tournament

        .import   _init_hgr, _init_mouse
        .import   mouse_x, mouse_y
        .import   mouse_dx, mouse_dy
        .import   _mouse_setbarbox, _mouse_setplaybox

        .import   _choose_opponent, _show_hall_of_fame

        .import   puck_x, puck_y, puck_dx, puck_dy, bounces
        .import   _init_precise_x, _init_precise_y, _transform_puck_coords

        .import   my_pusher_x, my_pusher_y
        .import   their_pusher_x, their_pusher_y
        .import   their_pusher_dx, their_pusher_dy

        .import   _puck_reinit_my_order, _puck_reinit_their_order
        .import   puck_in_front_of_me, prev_puck_in_front_of_me
        .import   puck_in_front_of_them, prev_puck_in_front_of_them
        .import   _draw_screen, _clear_screen, _draw_scores
        .import   _move_puck, _puck_check_my_hit, _puck_check_their_hit
        .import   _move_my_pusher, _move_their_pusher

        .import   __OPPONENT_START__

        .import   _load_table_high, _backup_table_high, _backup_table, _restore_table
        .import   _load_lowcode, _load_lc, _load_opponent
        .import   _load_bar_high, _backup_bar_high, _restore_bar
        .import   _load_bar_code, _backup_bar_code, _restore_bar_code
        .import   _load_barsnd, _backup_barsnd, _restore_barsnd
        .import   _init_text_before_decompress, _cache_working
        .import   _play_bar

        .import   hz

        .import   _mouse_wait_vbl
        .import   _mouse_calibrate_hz

        .import   _platform_msleep
        .import   _build_hgr_tables

        .import   _init_text, _memcpy, _clrscr, _cputs, pushax, ___errno, _strerror
        .import   ___randomize

        .include  "apple2.inc"
        .include  "sprite.inc"
        .include  "puck_coords.inc"
        .include  "my_pusher_coords.inc"
        .include  "my_pusher0.gen.inc"
        .include  "constants.inc"
        .include  "opponent_file.inc"

.code

.proc _main
        ; Init mouse first thing (it flickers HGR on IIplus)
        ; But disable IRQs as cc65's handler is in LOWCODE,
        ; which is not yet loaded
        php
        sei
        jsr     _init_mouse
        bcc     :+                ; Do we have a mouse?

        ; No. That won't work. We need one.
        brk

        ; Load LOWCODE (and splash screen)
:       jsr     _load_lowcode
        bcc     :+
        brk

:       ; We can now enable IRQs, the handler is loaded
        plp

        ; Calibrate to get Hz
        jsr     _mouse_calibrate_hz

        ; Wait for first interrupt
        jsr     _mouse_wait_vbl

        jsr     _clrscr

        lda     #1
        jsr     _init_hgr

        jmp     _real_main
.endproc

.segment "LOWCODE"

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

.proc _real_main
        lda     #<load_splc_str
        ldx     #>load_splc_str
        jsr     _cputs
        jsr     _load_lc
        bcc     :+
        jmp     load_error

:       jsr     ___randomize

        jsr     _build_hgr_tables

        ; Debug - start game at first opponent:
.ifdef DEBUG_OPPONENT
        lda     #1
        jsr     _init_hgr
        lda     #DEBUG_OPPONENT
        jmp     store_opponent
.endif
        ; End of debug

        ; Preload assets at $4000 and back them up
        ; while we have the splash screen at $2000
        lda     #<load_bar_str
        ldx     #>load_bar_str
        jsr     _cputs
        jsr     _load_bar_high
        bcc     :+
        jmp     load_error

:       jsr     _backup_bar_high

        ; Don't bother preloading the rest if /RAM is unavailable
        lda     _cache_working
        beq     new_opponent

        lda     #<load_bar_code_str
        ldx     #>load_bar_code_str
        jsr     _cputs
        jsr     _load_bar_code
        bcc     :+
        jmp     load_error

:       jsr     _backup_bar_code

        lda     #<load_table_str
        ldx     #>load_table_str
        jsr     _cputs
        jsr     _load_table_high
        bcc     :+
        jmp     load_error

:       jsr     _backup_table_high

        lda     #<load_barsnd_str
        ldx     #>load_barsnd_str
        jsr     _cputs

        jsr     _clrscr

        ; Remove splashscreen as the intro sound
        ; overwrites it
        ; Ask data loader do it as late as possible
        lda     #1
        sta     _init_text_before_decompress
        jsr     _load_barsnd
        bcc     :+
        jmp     load_error

:       jsr     _backup_barsnd
        lda     #0
        sta     _init_text_before_decompress

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
        jsr     _clrscr
        jsr     _init_text
        jsr     _restore_barsnd
        ldy     #0
        jsr     _play_bar
        jsr     _restore_bar
        jsr     _restore_bar_code

        lda     #1
        jsr     _init_hgr

        jsr     _mouse_setbarbox
        lda     won_tournament
        beq     :+

        jsr     _show_hall_of_fame

        lda     #0
        sta     won_tournament

:       jsr     _choose_opponent
        bpl     :+
        lda     #1                ; Start a tournament
        sta     in_tournament
        lda     #7                ; Start with opponent 0 FIXME
store_opponent:
:       sta     opponent

new_game:
        jsr     _mouse_setplaybox
        lda     #$00
        sta     turn
        sta     my_score
        sta     their_score

        lda     turn_puck_y
        sta     puck_serve_y

        ; Load the opponent file
        lda     opponent
        jsr     _load_opponent

        jsr     _restore_table
        jsr     _draw_opponent_parts
        jsr     _backup_table
        bcs     draw_scores

new_point:
        jsr     _restore_table
        bcc     draw_scores
        jsr     _draw_opponent_parts

draw_scores:
        ; Draw scores
        jsr     _draw_scores

        ; Check for end of game
        lda     #MAX_SCORE
        cmp     my_score
        beq     my_win
        cmp     their_score
        bne     cont_game

their_win:
        lda     #0                ; We lost the tournament
        sta     in_tournament
my_win:
        ; Todo victory/defeat screen
        lda     #<1000
        ldx     #>1000
        jsr     _platform_msleep
next_or_new_opponent:
        jsr     _clear_screen
        jsr     _restore_table
        jmp     new_opponent

clear_and_go_bar:
        lda     #0
        sta     in_tournament
        sta     won_tournament
        beq     next_or_new_opponent    ; Always branch

cont_game:

        ; Initialize coords
        jsr     _move_my_pusher

        ldy     #THEIR_PUSHER_INI_Y
        sty     their_pusher_y
        ldx     #THEIR_PUSHER_INI_X
        stx     their_pusher_x
        jsr     _move_their_pusher

        lda     #PUCK_INI_X
        sta     puck_x
        jsr     _init_precise_x
        lda     puck_serve_y
        sta     puck_y
        jsr     _init_precise_y

        ; Set correct graphics coords first
        jsr     _transform_puck_coords

        lda     #$00
        sta     puck_dx
        sta     puck_dy
        lda     #$01
        sta     serving

        jsr     _puck_reinit_my_order
        lda     puck_in_front_of_me
        sta     prev_puck_in_front_of_me
        jsr     _puck_reinit_their_order
        lda     puck_in_front_of_them
        sta     prev_puck_in_front_of_them

        lda     #6
        sta     frame_counter

game_loop:
        ; the WAI of the poor
        ; because I don't understand how WAI works
        ; and I want to keep things 6502-ok
        jsr     _mouse_wait_vbl

        ; Drop 1 frame out of 6 at 60Hz
        ; otherwise the game is harder
check_hz:
        lda     hz
        cmp     #60
        bne     loop_start
        dec     frame_counter
        bne     loop_start
        lda     #6
        sta     frame_counter
        bne     game_loop

loop_start:
        jsr     _draw_screen

        lda     puck_dy
        beq     :+
        lda     #$00
        sta     serving
:
        jsr     _move_my_pusher

        jsr     __OPPONENT_START__+OPPONENT::THINK_CB
        jsr     _move_their_pusher

        jsr     _puck_check_my_hit
        jsr     _puck_check_their_hit

        jsr     _move_puck
        bcs     reset_point

        ; Check again after moving puck
        jsr     _puck_check_my_hit

        jsr     _check_keyboard
        bcc     game_loop

        ; Keyboard hit, is it escape?
        cmp     #CH_ESC
        bne     :+
        jmp     clear_and_go_bar
:       cmp     #'w'
        bne     game_loop
        jmp     my_win
reset_point:
        lda     turn
        eor     #$01
        sta     turn
        tax
        lda     turn_puck_y,x
        sta     puck_serve_y

        ; Fix double-substraction of pushers height
        jsr     _move_my_pusher
        jsr     _move_their_pusher

reset_point_cont:
        jsr     _puck_reinit_my_order
        jsr     _puck_reinit_their_order

        lda     #$00
        sta     puck_dx
        sta     puck_dy
        sta     bounces

        lda     puck_x
        cmp     #PUCK_INI_X
        beq     reset_move_y
        bcc     :+
        lda     #<(-2)
        sta     puck_dx
        jmp     reset_move_y
:       lda     #2
        sta     puck_dx

reset_move_y:
        lda     puck_y
        cmp     puck_serve_y
        beq     update_screen
        bcc     :+
        lda     #<(-2)
        sta     puck_dy
        jmp     update_screen
:       lda     #2
        sta     puck_dy

update_screen:
        jsr     _mouse_wait_vbl
        jsr     _draw_screen
        jsr     _move_my_pusher
        jsr     _move_their_pusher
        jsr     _move_puck
        lda     puck_y
        cmp     puck_serve_y
        bne     reset_point_cont
        lda     puck_x
        cmp     #PUCK_INI_X
        bne     reset_point_cont

        ; Prepare for service
        lda    #0
        sta    their_pusher_dx
        sta    their_pusher_dy
        jsr     _clear_screen
        jmp     new_point
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

.proc load_error
        lda     ___errno
        ldx     #$00
        jsr     _strerror
        jsr     _cputs
        jsr     _init_text
        brk
.endproc

.data

turn_puck_y:
        .byte   MY_PUCK_INI_Y
        .byte   THEIR_PUCK_INI_Y

load_splc_str:    .byte "LOADING LC CODE..."       ,$0D,$0A,$00
load_table_str:   .byte "LOADING TABLE IMAGE..."   ,$0D,$0A,$00
load_barsnd_str:  .byte "LOADING INTRO SOUND..."   ,$0D,$0A,$00
load_bar_str:     .byte "LOADING BAR IMAGE..."     ,$0D,$0A,$00
load_bar_code_str:.byte "LOADING BAR CODE..."      ,$0D,$0A,$00
load_err_str:     .byte "COULD NOT LOAD FILE: "    ,$0D,$0A,$00

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
