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
; LEXAN
; How to beat: wait until he drank enough

        .import     their_pusher_x, their_pusher_y, _move_their_pusher
        .import     their_pusher_dx, their_pusher_dy
        .import     their_currently_hitting
        .import     puck_x, puck_right_x, puck_y, puck_dy, serving
        .import     _rand

        .import     _draw_screen
        .import     _mouse_wait_vbl

        .import     _big_draw_sprite_d                              ; CHANGE A
        .import     _big_draw_name_d                                ; CHANGE A
        .import     _big_draw_lose_d                                ; CHANGE A
        .import     _big_draw_win_d_1                               ; CHANGE A
        .import     _big_draw_win_d_2                               ; CHANGE A
        .import     _big_draw_win_d_3                               ; CHANGE A
        .import     _big_draw_win_d_4                               ; CHANGE A
        .import     _play_lose_d                                    ; CHANGE A

        .import     _platform_msleep

        .import     return0

        .import     __OPPONENT_START__
        .importzp   tmp1

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

START_MAX_DX = 11

.segment "d"                                                        ; CHANGE A

.assert * = __OPPONENT_START__+OPPONENT::SPRITE, error ; Make sure the callback is where we think
        jmp    _big_draw_sprite_d                                      ; CHANGE A

.assert * = __OPPONENT_START__+OPPONENT::NAME, error ; Make sure the callback is where we think
        jmp    _big_draw_name_d                                        ; CHANGE A

.assert * = __OPPONENT_START__+OPPONENT::LOSE_POINT, error ; Make sure the callback is where we think
        jmp     show_lose

.assert * = __OPPONENT_START__+OPPONENT::LOSE_POINT_SND, error ; Make sure the callback is where we think
        jmp     sound_lose

.assert * = __OPPONENT_START__+OPPONENT::WIN_POINT, error ; Make sure the callback is where we think
        jmp     return0

.assert * = __OPPONENT_START__+OPPONENT::WIN_POINT_SND, error ; Make sure the callback is where we think
        jmp     animate_win

.assert * = __OPPONENT_START__+OPPONENT::END_GAME, error ; Make sure the callback is where we think
        jmp     return0

.assert * = __OPPONENT_START__+OPPONENT::THINK_CB, error ; Make sure the callback is where we think
.proc _opponent_think
        lda     serving
        beq     serve_or_catch

init_service:
        cmp     #$01
        bne     prepare_service

        ; Adapt our speed because we drank when we won points
        lda     drink
        cmp     #6
        bcc     :+
        lda     #6
:       sta     tmp1
        lda     #START_MAX_DX
        sec
        sbc     tmp1
        sta     their_max_dx

        ; Who serves?
        lda     puck_y
        cmp     #THEIR_PUCK_INI_Y
        bne     serve_or_catch    ; It's the player

        ; Init serve parameters
        lda     their_max_dx
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
        GET_DELTA_TO_PUCK
        ; Bind to max dx
        BIND_SIGNED their_max_dx

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
        ; And make it 10-25 (=> 1-3 puck_dy)
        adc     #10
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

.proc prepare_animation
        jsr     _mouse_wait_vbl   ; ~45ms
        jsr     _mouse_wait_vbl
        jsr     _mouse_wait_vbl
        ldx     #((21+98)/7)
        ldy     #(76)
        rts
.endproc

.proc animate_win
        inc     drink
        lda     #THEIR_PUSHER_MIN_X
        sta     their_pusher_x
        lda     #THEIR_PUSHER_MIN_Y
        sta     their_pusher_y
        jsr     _move_their_pusher
        jsr     _mouse_wait_vbl
        jsr     _draw_screen

        jsr     prepare_animation
        jsr     _big_draw_win_d_1

        jsr     prepare_animation
        jsr     _big_draw_win_d_2

        jsr     prepare_animation
        jsr     _big_draw_win_d_3

        jsr     prepare_animation
        jsr     _big_draw_win_d_4
        lda     #<700
        ldx     #>700
        jsr     _platform_msleep

        jsr     prepare_animation
        jsr     _big_draw_win_d_3

        jsr     prepare_animation
        jsr     _big_draw_win_d_2

        jsr     prepare_animation
        jmp     _big_draw_win_d_1
.endproc

.proc show_lose
        ldx     #((21+98)/7)
        ldy     #(57)
        jmp     _big_draw_lose_d                                        ; CHANGE A
.endproc

.proc sound_lose
        ldy     #0
        jmp     _play_lose_d                                            ; CHANGE A
.endproc

their_max_dx:     .byte START_MAX_DX
drink:            .byte 0
