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
; BEJIN
; How to beat: Listen to her serve

        .import     their_pusher_x, their_pusher_y, my_pusher_y
        .import     their_pusher_dx, their_pusher_dy
        .import     their_currently_hitting, player_caught
        .import     puck_x, puck_right_x, puck_y, puck_dx, puck_dy, serving, their_score
        .import     _guess_puck_x_at_y, bounces
        .import     _rand, _platform_msleep
        .import     _draw_opponent

        .import     _mouse_wait_vbl

        .import     _big_draw_sprite_g                              ; CHANGE A
        .import     _big_draw_name_g                                ; CHANGE A
        .import     _big_draw_serve_g_1                             ; CHANGE A
        .import     _big_draw_serve_g_2                             ; CHANGE A
        .import     _big_draw_serve_g_3                             ; CHANGE A
        .import     _big_draw_serve_g_4                             ; CHANGE A
        .import     _big_draw_win_g                                 ; CHANGE A
        .import     _play_serve_g_left                              ; CHANGE A
        .import     _play_serve_g_right

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

HIT_Y = 25
START_SERVICE_DELTA = <8
END_SERVICE_DY = <20
END_SERVICE_LEFT_DX = <-40
END_SERVICE_RIGHT_DX = <12

.segment "g"                                                            ; CHANGE A

.assert * = __OPPONENT_START__+OPPONENT::SPRITE, error ; Make sure the callback is where we think
        jmp     _big_draw_sprite_g                                      ; CHANGE A

.assert * = __OPPONENT_START__+OPPONENT::NAME, error ; Make sure the callback is where we think
        jmp     _big_draw_name_g                                        ; CHANGE A

.assert * = __OPPONENT_START__+OPPONENT::LOSE_POINT, error ; Make sure the callback is where we think
        jmp     return0

.assert * = __OPPONENT_START__+OPPONENT::LOSE_POINT_SND, error ; Make sure the callback is where we think
        jmp     return0

.assert * = __OPPONENT_START__+OPPONENT::WIN_POINT, error ; Make sure the callback is where we think
        jmp     win_animation

.assert * = __OPPONENT_START__+OPPONENT::WIN_POINT_SND, error ; Make sure the callback is where we think
        jmp     return0

.assert * = __OPPONENT_START__+OPPONENT::END_GAME, error ; Make sure the callback is where we think
        jmp     return0

.assert * = __OPPONENT_START__+OPPONENT::THINK_CB, error ; Make sure the callback is where we think
.proc _opponent_think
        lda     serving
        beq     catch

init_service:
        cmp     #$01
        bne     prepare_service

        lda     #$00
        sta     found_x

        jsr     _rand
        lsr
        lsr
        lsr
        lsr
        lsr
        sta     magic_band_rnd

        inc     serving           ; init service only once

        lda     puck_y            ; Who serves?
        cmp     #THEIR_PUCK_INI_Y
        bne     :+
        jmp     init_opponent_service
:       lda     #1
        sta     service_finished   ; Don't hack the puck on player service
        rts


prepare_service:
        lda     puck_y            ; Who serves?
        cmp     #THEIR_PUCK_INI_Y
        beq     :+
        rts
:       jmp     prepare_opponent_service

catch:
        lda     service_finished
        bne     :+
        jsr     hack_service

:       ; Is it time to figure out destination X?
        lda     puck_dy
        bpl     move

        lda     puck_y
        clc
        adc     magic_band_rnd
        ; Magic band where guessing X is allowed, 130-122 with a random 8 offset
        cmp     #129
        bcs     move
        cmp     #121
        bcc     move

        ; Figure X if not already done
        lda     found_x
        bne     move
        lda     #HIT_Y
        jsr     _guess_puck_x_at_y

        ; And store it (but not 0, so we know we found it)
        tax
        bne     :+
        inx
:       stx     found_x

move:
        lda     found_x
        beq     follow_puck_or_wait

        ; Go directly where the puck will be when hittable
        lda     found_x
        cmp     #puck0_WIDTH
        bcs     :+
        lda     #0
        beq     store_x
:       cmp     #(PUCK_MAX_X-puck0_WIDTH)
        bcc     store_x
        lda     #THEIR_PUSHER_MAX_X-1
        bne     store_x

        sec
        sbc     #((my_pusher0_WIDTH-puck0_WIDTH)/2)
store_x:
        sta     their_pusher_x

        lda     #HIT_Y-10
        sta     their_pusher_y

        lda     #$00
        jmp     store_dx

follow_puck_or_wait:
        lda     puck_dy
        bmi     follow_puck
        rts

follow_puck:
        GET_DELTA_TO_PUCK
        ; Bind to max dx
        BIND_SIGNED their_max_dx
store_dx:
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
        ; Get a 0-15 DY if we didn't find X
        jsr     _rand
        lsr
        lsr
        lsr
        lsr
        clc
        ; And make it 10-25 (=> 1-3 puck_dy)
        adc     #10

        sta     their_pusher_dy
        lda     #0
        sta     their_pusher_dx
        ; Forget found X
        sta     found_x
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

.proc init_opponent_service
        lda     #0
        sta     their_pusher_dx
        sta     their_pusher_dy

        lda     #(PUCK_INI_X-(my_pusher0_WIDTH/2)+(puck0_WIDTH/2))
        sta     their_pusher_x
        lda     #THEIR_PUSHER_MIN_Y+1
        sta     their_pusher_y

        ; Shorten wait and make it predictible
        lda     #150
        sta     serving

        lda     #1
        sta     preparing_service
        lda     #0
        sta     serve_hand_phase
        rts
.endproc

.proc prepare_opponent_service
        lda     preparing_service
        cmp     #1
        beq     go_front
        cmp     #2
        beq     raise_hand
        cmp     #3
        beq     choose_direction
        cmp     #4
        beq     suspense
        cmp     #5
        beq     prepare_to_hit
        rts                       ; All's ready

go_front:
        lda     their_pusher_y
        cmp     #THEIR_PUCK_INI_Y-3
        bcs     preparation_done
        inc     their_pusher_y
        rts

raise_hand:
        lda     serve_hand_phase
        cmp     #4
        beq     preparation_done
        jmp     update_hand_sprite

choose_direction:
        lda     #END_SERVICE_RIGHT_DX
        sta     serve_direction

        ; Randomize left/right
        jsr     _rand
        and     #1
        beq     :+

        ; Serve left
        lda     #END_SERVICE_LEFT_DX
        sta     serve_direction

        ldy     #0
        jsr     _play_serve_g_left                                   ; CHANGE A

:       ldy     #0
        jsr     _play_serve_g_right                                  ; CHANGE A
        ldy     #0
        jsr     _play_serve_g_right                                  ; CHANGE A
        jmp     preparation_done

prepare_to_hit:
        ; Back to normal sprite
        jsr     _draw_opponent

        lda     #$00
        sta     their_pusher_dx
        sta     their_pusher_dy
        sta     service_finished
        sta     service_hack_phase
        lda     #START_SERVICE_DELTA
        sta     puck_dy
        jmp     preparation_done

suspense:
        lda     serving
        cmp     #$FF
        beq     preparation_done
        inc     serving
        rts

preparation_done:
        inc     preparing_service
        rts
.endproc

.proc hack_service
        lda     service_hack_phase
        cmp     #0
        beq     go_mid_board_y
        cmp     #1
        beq     go_right
        cmp     #2
        beq     launch
        rts

go_mid_board_y:
        lda     #0
        sta     puck_dx
        lda     #START_SERVICE_DELTA
        sta     puck_dy

        lda     puck_y
        cmp     #MID_BOARD
        bcs     inc_hack
        rts

go_right:                           ; Move puck to 3/4 X
        lda     #0
        sta     puck_dy
        lda     #START_SERVICE_DELTA
        sta     puck_dx

        lda     puck_x
        cmp     #((PUCK_MAX_X*3)/4)
        bcs     inc_hack
        rts

launch:                           ; Let's go!
        lda     #END_SERVICE_DY
        sta     puck_dy
        sta     service_finished

        lda     serve_direction
        sta     puck_dx
        
inc_hack:
        inc     service_hack_phase
        rts
.endproc

.proc update_hand_sprite
        inc     serve_hand_phase
        lda     #50
        ldx     #0
        jsr     _platform_msleep

        jsr     _mouse_wait_vbl

        ldx     #((7+98)/7)
        ldy     #76

        lda     serve_hand_phase
        cmp     #1
        beq     serve_hand_1
        cmp     #2
        beq     serve_hand_2
        cmp     #3
        beq     serve_hand_3
        cmp     #4
        beq     serve_hand_4
        rts
serve_hand_1:
        jmp     _big_draw_serve_g_1                                   ; CHANGE A
serve_hand_2:
        jmp     _big_draw_serve_g_2                                   ; CHANGE A
serve_hand_3:
        jmp     _big_draw_serve_g_3                                   ; CHANGE A
serve_hand_4:
        jmp     _big_draw_serve_g_4                                   ; CHANGE A
.endproc

.proc win_animation
        ldx     #((35+98)/7)    ; left X of sprite change + left X of big sprite
        ldy     #(38)           ; bottom Y of sprite change
        jmp     _big_draw_win_g                                        ; CHANGE A
.endproc

their_max_dx:      .byte 10
their_max_dy:      .byte 4
mid_pusher_x:      .byte 1
found_x:           .byte 0
preparing_service: .byte 0
magic_band_rnd:    .byte 0
serve_direction:   .byte 0
service_finished:  .byte 0
service_hack_phase:.byte 0
serve_hand_phase:  .byte 0
