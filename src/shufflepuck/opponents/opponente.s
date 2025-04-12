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
; ENEG
; How to beat: wear him down and aim for the corners

        .import     their_pusher_x, their_pusher_y
        .import     their_pusher_dx, their_pusher_dy
        .import     player_caught, their_currently_hitting
        .import     puck_x, puck_right_x, puck_y, puck_dy, serving, their_score
        .import     _rand

        .import     _big_draw_sprite_e                              ; CHANGE A
        .import     _big_draw_name_e                                ; CHANGE A
        .import     _big_draw_normal_e                              ; CHANGE A
        .import     _big_draw_lose_e                                ; CHANGE A
        .import     _big_draw_win_e                                 ; CHANGE A
        .import     _play_win_e                                     ; CHANGE A
        .import     _play_lose_e                                    ; CHANGE A

        .import     __OPPONENT_START__
        .importzp   tmp1

        .include    "helpers.inc"
        .include    "apple2.inc"
        .include    "my_pusher0.gen.inc"
        .include    "puck0.gen.inc"
        .include    "../code/sprite.inc"
        .include    "../code/their_pusher_coords.inc"
        .include    "../code/puck_coords.inc"
        .include    "../code/constants.inc"
        .include    "../code/opponent_file.inc"

THEIR_MAX_DX = 16
START_MIN_X = THEIR_PUSHER_MIN_X
START_MAX_X = THEIR_PUSHER_MAX_X
TOO_FAST_DY = -10
MAX_NUM_CATCH = 18

.segment "e"                                                        ; CHANGE A

.assert * = __OPPONENT_START__+OPPONENT::SPRITE, error ; Make sure the callback is where we think
sprite:
        ldx     #(98/7)
        ldy     #76
        jmp    _big_draw_sprite_e                                      ; CHANGE A

.assert * = __OPPONENT_START__+OPPONENT::NAME, error ; Make sure the callback is where we think
name:
        ldx     #(7/7)
        ldy     #39
        jmp    _big_draw_name_e                                        ; CHANGE A

.assert * = __OPPONENT_START__+OPPONENT::LOSE_POINT, error ; Make sure the callback is where we think
lose_animation:
        ldx     #((98+21)/7)
        ldy     #(64+1)
        jmp    _big_draw_lose_e                                        ; CHANGE A

.assert * = __OPPONENT_START__+OPPONENT::LOSE_POINT_SND, error ; Make sure the callback is where we think
lose_sound:
        ldy     #0
        jmp     _play_lose_e                                            ; CHANGE A

.assert * = __OPPONENT_START__+OPPONENT::WIN_POINT, error ; Make sure the callback is where we think
win_animation:
        ldx     #((98+21)/7)
        ldy     #(64+1)
        jmp    _big_draw_win_e                                        ; CHANGE A

.assert * = __OPPONENT_START__+OPPONENT::WIN_POINT_SND, error ; Make sure the callback is where we think
win_sound:
        ldy     #0
        jmp     _play_win_e                                            ; CHANGE A

.assert * = __OPPONENT_START__+OPPONENT::THINK_CB, error ; Make sure the callback is where we think
.proc _opponent_think
        lda     serving
        beq     serve_or_catch

init_service:
        cmp     #$01
        bne     prepare_service

        ; Reset exchange
        lda     #0
        sta     num_catch

        ; Who serves?
        lda     puck_y
        cmp     #THEIR_PUCK_INI_Y
        bne     serve_or_catch    ; It's the player

        ; Init serve parameters
        lda     #THEIR_MAX_DX
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

        lda     #$01
        sta     their_pusher_dy
        jsr     invert_pusher_dx

        rts
:       cmp     #(THEIR_PUCK_INI_Y-2)
        bcs     :+
        rts
:

        lda     #$FF
        sta     their_pusher_dy
        jsr     invert_pusher_dx
        rts

serve_or_catch:
        lda     puck_dy           ; Is the puck moving?
        bne     catch

        lda     #$00              ; Cancel Y speed
        sta     their_pusher_dy

catch:
        ; How many times did the player catch the puck?
        lda     player_caught
        beq     :+

        lda     #0              ; Reset indicator
        sta     player_caught

        lda     num_catch       ; Stop incrementing num_catch at MAX_NUM_CATCH
        cmp     #MAX_NUM_CATCH  ; So that it doesn't get too easy
        bcs     :+
        inc     num_catch

:       ; Is the puck's DY fast enough to miss?
        lda     puck_dy
        cmp     #<(TOO_FAST_DY)
        bcs     not_fast_enough

        lda     num_catch         ; Lose 2 pixels per exchange
        asl
        sta     tmp1
        lda     #START_MAX_X      ; Decrement on the right
        sec
        sbc     tmp1
        sta     max_x
        lda     #START_MIN_X      ; Increment on the left
        clc
        adc     tmp1
        sta     min_x
        bne     do_catch

not_fast_enough:
        lda     #START_MAX_X
        sta     max_x
        lda     #START_MIN_X
        sta     min_x

do_catch:
        lda     their_pusher_x
        clc                       ; Center puck on pusher
        adc     #((my_pusher0_WIDTH-puck0_WIDTH)/2)
        sta     mid_pusher_x
        sec
        cmp     puck_x
        bcs     move_left

move_right:
        lda     puck_x
        sec
        sbc     mid_pusher_x

        cmp     #THEIR_MAX_DX
        bcc     :+
        lda     #THEIR_MAX_DX
        clc

:       ; Check if we would go to far
        sta     tmp1
        clc
        adc     their_pusher_x
        bcc     :+
        lda     max_x
:       cmp     max_x
        bcc     store_dx
        lda     #0
        sta     tmp1
        jmp     store_dx

move_left:
        lda     mid_pusher_x
        sec
        sbc     puck_x

        cmp     #THEIR_MAX_DX
        bcc     :+
        lda     #THEIR_MAX_DX

:       NEG_A

        ; Check if we would go to far
        sta     tmp1
        clc
        adc     their_pusher_x
        bcs     :+
        lda     min_x
:       cmp     min_x
        bcs     store_dx
        lda     #0
        sta     tmp1

store_dx:
        lda     tmp1
        sta     their_pusher_dx

        ; Did we just hit?
        lda     their_currently_hitting
        bne     move_backwards

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
        jsr     _rand
        lsr
        lsr
        lsr
        lsr
        clc
        ; And make it 9-24
        adc     #9
        sta     their_pusher_dy
        rts

move_forwards_slow:
        lda     #3
        sta     their_pusher_dy
        rts

move_backwards:
        lda     #<-6
        sta     their_pusher_dy
        rts
.endproc

.proc invert_pusher_dx
        lda     their_pusher_dx
        NEG_A
        sta     their_pusher_dx
        rts
.endproc

min_x:            .byte 1
max_x:            .byte 1
num_catch:        .byte 1
mid_pusher_x:     .byte 1
