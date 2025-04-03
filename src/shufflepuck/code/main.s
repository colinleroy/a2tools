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

        .import   _init_hgr, _init_mouse
        .import   mouse_x, mouse_y
        .import   mouse_dx, mouse_dy

        .import   puck_x, puck_y, puck_dx, puck_dy
        .import   _transform_puck_coords

        .import   my_pusher_x, my_pusher_y
        .import   their_pusher_x, their_pusher_y
        .import   their_pusher_dx, their_pusher_dy

        .import   _puck_reinit_my_order, _puck_reinit_their_order
        .import   _draw_screen, _clear_screen, _draw_scores
        .import   _move_puck, _puck_check_my_hit, _puck_check_their_hit
        .import   _move_my_pusher, _move_their_pusher

        .import   __OPPONENT_START__

        .import   _load_table, _backup_table, _restore_table
        .import   _load_lowcode, _load_opponent
        .import   hz

        .import   _mouse_wait_vbl
        .import   _mouse_reset
        .import   _mouse_calibrate_hz

        .import   _platform_msleep
        .import   _allow_lowercase
        .import   _build_hgr_tables

        .import   _init_text, _clrscr, _memcpy, pushax

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

.proc _real_main
        jsr     _build_hgr_tables

        lda     #1
        jsr     _allow_lowercase  ; For Bulgarian i18n

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

        jsr     _load_table

        lda     #1
        jsr     _init_hgr

        jsr     _mouse_reset

        ; Wait for first interrupt
        jsr     _mouse_wait_vbl

new_game:
        lda     #$00
        sta     turn
        sta     my_score
        sta     their_score

        lda     turn_puck_y
        sta     puck_serve_y


        ; Load the opponent file
        lda     #0
        jsr     _load_opponent

        ; Draw its sprite
        lda     #OPPONENT::SPRITE
        ldx     #(98/7)
        ldy     #76
        jsr     draw_opponent_part

        ; Draw its name
        lda     #OPPONENT::NAME
        ldx     #(7/7)
        ldy     #39
        jsr     draw_opponent_part

        jsr     _backup_table

new_point:
        ; Draw scores
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
        jmp     new_game

cont_game:

        ; Initialize coords
        ldy     mouse_y
        sty     my_pusher_y
        ldx     mouse_x
        stx     my_pusher_x

        ldy     #THEIR_PUSHER_INI_Y
        sty     their_pusher_y
        ldx     #THEIR_PUSHER_INI_X
        stx     their_pusher_x

        lda     #PUCK_INI_X
        sta     puck_x
        lda     puck_serve_y
        sta     puck_y
        ; Set correct graphics coords first
        jsr     _transform_puck_coords

        lda     #$00
        sta     puck_dx
        sta     puck_dy
        lda     #$01
        sta     serving

        jsr     _puck_reinit_my_order
        jsr     _puck_reinit_their_order

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

        ; Next round!
        jmp     game_loop

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
        dec     puck_dx
        jmp     reset_move_y
:       inc     puck_dx

reset_move_y:
        lda     puck_y
        cmp     puck_serve_y
        beq     update_screen
        bcc     :+
        dec     puck_dy
        jmp     update_screen
:       inc     puck_dy

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
        jsr     _restore_table
        jmp     new_point
.endproc

.data

turn_puck_y:
        .byte   MY_PUCK_INI_Y
        .byte   THEIR_PUCK_INI_Y

.bss

turn:         .res 1
puck_serve_y: .res 1
serving:      .res 1

my_score:     .res 1
their_score:  .res 1
