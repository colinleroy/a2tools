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

; ----------
; VISINE
; How to beat: Aim for corners hard enough

        .import     their_pusher_x, their_pusher_y
        .import     their_pusher_dx, their_pusher_dy
        .import     their_currently_hitting
        .import     puck_x, puck_right_x, puck_y, puck_dx, puck_dy, serving, their_score
        .import     _rand

        .import     _big_draw_sprite_b                              ; CHANGE A
        .import     _big_draw_name_b                                ; CHANGE A
        .import     _big_draw_lose_b                                ; CHANGE A
        .import     _big_draw_win_b                                 ; CHANGE A

        .import     _play_win_b, _play_losegame_b, _platform_msleep

        .import     return0

        .import     __OPPONENT_START__

        .include    "code/helpers.inc"
        .include    "opponent_helpers.inc"
        .include    "apple2.inc"
        .include    "my_pusher0.gen.inc"
        .include    "puck0.gen.inc"
        .include    "../code/sprite.inc"
        .include    "../code/their_pusher_coords.inc"
        .include    "../code/puck_coords.inc"
        .include    "../code/constants.inc"
        .include    "../code/opponent_file.inc"

THEIR_MAX_DX = 15
THEIR_MAX_DY = 8
.segment "b"                                                        ; CHANGE A

.assert * = __OPPONENT_START__+OPPONENT::SPRITE, error ; Make sure the callback is where we think
        jmp    _big_draw_sprite_b                                      ; CHANGE A

.assert * = __OPPONENT_START__+OPPONENT::NAME, error ; Make sure the callback is where we think
        jmp     _big_draw_name_b                                        ; CHANGE A

.assert * = __OPPONENT_START__+OPPONENT::LOSE_POINT, error ; Make sure the callback is where we think
        jmp     animate_lose

.assert * = __OPPONENT_START__+OPPONENT::LOSE_POINT_SND, error ; Make sure the callback is where we think
        jmp     return0

.assert * = __OPPONENT_START__+OPPONENT::WIN_POINT, error ; Make sure the callback is where we think
        jmp     animate_win

.assert * = __OPPONENT_START__+OPPONENT::WIN_POINT_SND, error ; Make sure the callback is where we think
        jmp     sound_win

.assert * = __OPPONENT_START__+OPPONENT::END_GAME, error ; Make sure the callback is where we think
        jmp     end_game

.assert * = __OPPONENT_START__+OPPONENT::HIT_CB, error
        jmp     return0

.assert * = __OPPONENT_START__+OPPONENT::THINK_CB, error ; Make sure the callback is where we think
.proc _opponent_think
        lda     serving
        beq     serve_or_catch

init_service:
        cmp     #$01
        bne     prepare_service

        ; Who serves?
        lda     puck_y
        cmp     #THEIR_PUCK_INI_Y
        bne     serve_or_catch    ; It's the player

        ; Init serve parameters
        lda     #0
        sta     their_pusher_dx

        ; Shorten wait
        jsr     _rand
        beq     prepare_service
        sta     serving

prepare_service:
        ; We're serving
        inc     serving
        beq     serve_or_catch      ; After a while we got to serve

        lda     their_pusher_y
        cmp     #(THEIR_PUSHER_MIN_Y+1)
        bcs     :+

        lda     #0
        sta     their_pusher_dx
        lda     #THEIR_MAX_DY
        sta     their_pusher_dy
        rts

:       cmp     #(THEIR_PUCK_INI_Y-2)
        bcs     :+
        rts
:

        lda     #$FF
        sta     their_pusher_dy
        rts

serve_or_catch:
        lda     puck_dy           ; Is the puck moving?
        bne     catch_or_move

catch_or_move:
        lda     puck_y
        cmp     #70
        bcc     catch
        jmp     move_fast

catch:
        GET_DELTA_TO_PUCK
        ; Bind to max dx
        BIND_SIGNED #THEIR_MAX_DX
        sta     their_pusher_dx

        jsr     bind_x

        ; Did we just hit?
        lda     their_currently_hitting
        bne     add_dx

        ; Can we hit now?
        lda     their_pusher_x
        cmp     puck_right_x
        bcs     move_backwards

        clc
        adc     #my_pusher0_WIDTH ; Same for their pusher, they are the same size
        bcs     :+                ; If adding our width overflowed, we're good
        cmp     puck_x            ; the pusher is on the right and the puck too
        bcc     move_backwards

:       lda     puck_y
        cmp     #MY_PUSHER_MAX_Y   ; Move forward as long as puck isn't in the end of the player zone (defensive)
        bcs     move_forwards_slow
        cmp     #(THEIR_PUSHER_MAX_Y)
        bcs     move_backwards
hit:
        ; Get a 0-15 DY
        UNSIGNED_RAND_0_15_A
        sta     their_pusher_dy
        rts

move_forwards_slow:
        lda     #(THEIR_MAX_DY/2)
        sta     their_pusher_dy
        rts

add_dx:
        cmp     #14               ; Did we just really hit on previous loop?
        bne     move_backwards
        ; Add dx
        SIGNED_RAND_0_15_A
        sta     puck_dx
move_backwards:
        ; And go backwards
        lda     #<(-THEIR_MAX_DY/2)
        sta     their_pusher_dy
        rts

move_fast:
        lda     their_pusher_dx
        beq     init_move_fast

        jmp     bind_x_y

init_move_fast:
        lda     #THEIR_MAX_DX
        sta     their_pusher_dx
        lda     #THEIR_MAX_DY
        sta     their_pusher_dy
        rts
.endproc

.proc revert_x
        lda     their_pusher_dx
        NEG_A
        sta     their_pusher_dx
        rts
.endproc

.proc revert_y
        lda     their_pusher_dy
        NEG_A
        sta     their_pusher_dy
        rts
.endproc

.proc bind_x
        lda     puck_dy           ; Don't bind for slow hits
        cmp     #<-8
        bcs     out

        lda     their_pusher_x
        cmp     #40
        bcc     revert_right
        cmp     #256-40-my_pusher0_WIDTH
        bcs     revert_left
out:    rts

revert_right:
        bit     their_pusher_dx
        bmi     do_revert
        rts

revert_left:
        bit     their_pusher_dx
        bmi     out
do_revert:
        jmp     revert_x
.endproc

.proc bind_x_y
        jsr     bind_x

        lda     their_pusher_y
        cmp     #THEIR_PUSHER_MIN_Y+2
        bcs     :+
        jsr     revert_y
:       cmp     #THEIR_PUSHER_MAX_Y
        bcc     :+
        jsr     revert_y
:       rts
.endproc

.proc load_simple_animation_coords
        ldx     #((7+OPPONENT_SPRITE_X)/7)    ; left X of sprite change + left X of big sprite
        ldy     #OPPONENT_SPRITE_Y-OPPONENT_SPRITE_H+63             ; bottom Y of sprite change
        rts
.endproc

.proc animate_lose
        jsr     load_simple_animation_coords
        jmp     _big_draw_lose_b                                        ; CHANGE A
.endproc

.proc animate_win
        jsr     load_simple_animation_coords
        jmp     _big_draw_win_b                                        ; CHANGE A
.endproc

.proc end_game
        lda     their_score ; Did we lose?
        cmp     #15
        bne     :+
        rts
:       ldy     #0
        jsr     _play_losegame_b
        lda     #100
        ldx     #0
        jsr     _platform_msleep
        ldy     #0
        jmp     _play_losegame_b
.endproc

.proc sound_win
        ldy     #0
        jmp     _play_win_b
.endproc
