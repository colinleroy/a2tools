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
        .export   _check_keyboard

        .import   _init_hgr, _init_mouse
        .import   mouse_x, mouse_y
        .import   mouse_dx, mouse_dy
        .import   _mouse_setbarbox, _mouse_setplaybox

        .import   _choose_opponent

        .import   puck_x, puck_y, puck_dx, puck_dy
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

        .import   _load_table, _backup_table, _restore_table
        .import   _load_lowcode, _load_lc, _load_opponent
        .import   _load_bar, _backup_bar, _restore_bar
        .import   _load_barsnd, _backup_barsnd, _restore_barsnd
        .import   _play_bar

        .import   hz

        .import   _mouse_wait_vbl
        .import   _mouse_calibrate_hz

        .import   _platform_msleep
        .import   _allow_lowercase
        .import   _build_hgr_tables

        .import   _init_text, _memcpy, _clrscr, _cputs, pushax
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
        jsr     _load_lowcode

        lda     #1
        jsr     _allow_lowercase

        jsr     _clrscr
        lda     #<load_splc_str
        ldx     #>load_splc_str
        jsr     _cputs
        jsr     _load_lc

        jsr     ___randomize
        jmp     _real_main
.endproc

.segment "LOWCODE"

; A: the part (SPRITE or NAME)
; X,Y, the lower left coordinate
.proc draw_opponent_part
        clc
        adc     #<(__OPPONENT_START__)
        sta     draw+1
        lda     #>(__OPPONENT_START__)
        adc     #0
        sta     draw+2

draw:
        jmp     $FFFF
.endproc

.proc draw_opponent_parts
        ; Draw its sprite
        lda     #OPPONENT::SPRITE
        jsr     draw_opponent_part
        ; Draw its name
        lda     #OPPONENT::NAME
        jmp     draw_opponent_part
.endproc

.proc _real_main
        jsr     _build_hgr_tables

        jsr     _init_mouse
        bcc     :+                ; Do we have a mouse?

        ; No. That won't work.
        brk

calibrate_hz_handler:
:       jsr     _mouse_calibrate_hz

.ifndef __APPLE2ENH__
        ; Give the Mousecard time to settle post-init
        lda     #$FF
        ldx     #0
        jsr     _platform_msleep
.endif

        lda     #<load_table_str
        ldx     #>load_table_str
        jsr     _cputs
        jsr     _load_table
        jsr     _backup_table

        ; Wait for first interrupt
        jsr     _mouse_wait_vbl

        lda     #<load_barsnd_str
        ldx     #>load_barsnd_str
        jsr     _cputs
        jsr     _load_barsnd
        jsr     _backup_barsnd

        lda     #<load_bar_str
        ldx     #>load_bar_str
        jsr     _cputs
        jsr     _load_bar
        jsr     _backup_bar

new_opponent:
        jsr     _clrscr
        jsr     _init_text
        jsr     _restore_barsnd
        ldy     #0
        jsr     _play_bar
        jsr     _restore_bar

        lda     #1
        jsr     _init_hgr

        jsr     _mouse_setbarbox
        jsr     _choose_opponent
        sta     opponent

new_game:
        jsr     _restore_table
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

        jsr     draw_opponent_parts
        jsr     _backup_table

new_point:
        jsr     _restore_table
        bcc     :+
        jsr     draw_opponent_parts

:       ; Draw scores
        jsr     _draw_scores

        ; Check for end of game
        lda     #MAX_SCORE
        cmp     my_score
        beq     my_win
        cmp     their_score
        bne     cont_game

their_win:
my_win:
        ; Todo victory/defeat screen
        lda     #<1000
        ldx     #>1000
        jsr     _platform_msleep
clear_and_new_opponent:
        jsr     _clear_screen
        jsr     _restore_table
        jmp     new_opponent

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

game_loop:
        ; the WAI of the poor
        ; because I don't understand how WAI works
        ; and I want to keep things 6502-ok
        jsr     _mouse_wait_vbl

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

        jsr     _check_keyboard
        bcc     game_loop

        ; Keyboard hit, is it escape?
        cmp     #CH_ESC
        beq     clear_and_new_opponent
        bne     game_loop

reset_point:
        lda     turn
        eor     #$01
        sta     turn
        tax
        lda     turn_puck_y,x
        sta     puck_serve_y

reset_point_cont:
        lda     #$00
        sta     puck_dx
        sta     puck_dy

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
        cmp     #CH_ESC
        beq     out_kbd
        
        cmp     #'0'
        bcc     no_kbd
        cmp     #('9'+1)
        bcs     no_kbd
        sec
        sbc     #'0'
        sta     opponent
out_kbd:
        sec
        rts

no_kbd:
        clc
        rts
.endproc

.data

turn_puck_y:
        .byte   MY_PUCK_INI_Y
        .byte   THEIR_PUCK_INI_Y

load_splc_str:    .byte "LOADING CODE..."          ,$0D,$0A,$00
load_table_str:   .byte "LOADING ASSETS..."        ,$0D,$0A,$00
load_barsnd_str:  .byte "DISMANTLING CAPITALISM...",$0D,$0A,$00
load_bar_str:     .byte "ANY MINUTE NOW..."        ,$0D,$0A,$00

.bss

turn:         .res 1
puck_serve_y: .res 1
serving:      .res 1

my_score:     .res 1
their_score:  .res 1
opponent:     .res 1
