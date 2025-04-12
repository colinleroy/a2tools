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

        .import     _big_draw_sprite_g                              ; CHANGE A
        .import     _big_draw_name_g                                ; CHANGE A
        .import     _big_draw_normal_g                              ; CHANGE A
        .import     _big_draw_lose_g                                ; CHANGE A
        .import     _big_draw_win_g                                 ; CHANGE A
        .import     _play_serve_g_left                              ; CHANGE A
        .import     _play_serve_g_right

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

.segment "g"                                                            ; CHANGE A

.assert * = __OPPONENT_START__+OPPONENT::SPRITE, error ; Make sure the callback is where we think
sprite:
        ldx     #(98/7)
        ldy     #76
        jmp     _big_draw_sprite_g                                      ; CHANGE A

.assert * = __OPPONENT_START__+OPPONENT::NAME, error ; Make sure the callback is where we think
name:
        ldx     #(7/7)
        ldy     #39
        jmp     _big_draw_name_g                                        ; CHANGE A

.assert * = __OPPONENT_START__+OPPONENT::LOSE_POINT, error ; Make sure the callback is where we think
lose_animation:
        ldx     #((00+98)/7)    ; left X of sprite change + left X of big sprite
        ldy     #(76)           ; bottom Y of sprite change
        jmp     _big_draw_lose_g                                        ; CHANGE A

.assert * = __OPPONENT_START__+OPPONENT::LOSE_POINT_SND, error ; Make sure the callback is where we think
lose_sound:
        rts
        .res 4

.assert * = __OPPONENT_START__+OPPONENT::WIN_POINT, error ; Make sure the callback is where we think
win_animation:
        ldx     #((00+98)/7)    ; left X of sprite change + left X of big sprite
        ldy     #(76)           ; bottom Y of sprite change
        jmp     _big_draw_win_g                                        ; CHANGE A

.assert * = __OPPONENT_START__+OPPONENT::WIN_POINT_SND, error ; Make sure the callback is where we think
win_sound:
        rts
        .res 4

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
:       jmp     init_patrol_delta  ; Patrol


prepare_service:
        lda     puck_y            ; Who serves?
        cmp     #THEIR_PUCK_INI_Y
        beq     :+
        jmp     patrol            ; Wait for player to serve
:       jmp     prepare_opponent_service

catch:
        ; Is it time to figure out destination X?
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
        ; Reinit patrol
        jsr     init_patrol_delta

        lda     found_x
        beq     follow_puck_or_patrol

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

follow_puck_or_patrol:
        lda     puck_dy
        bmi     follow_puck
        jmp     patrol

follow_puck:
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

        cmp     their_max_dx
        bcc     store_dx
        lda     their_max_dx
        clc
        jmp     store_dx

move_left:
        lda     mid_pusher_x
        sec
        sbc     puck_x

        cmp     their_max_dx
        bcc     :+
        lda     their_max_dx

:       NEG_A
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

.proc invert_pusher_dx
        lda     their_pusher_dx
        NEG_A
        sta     their_pusher_dx
        rts
.endproc

.proc patrol
        lda     their_pusher_y
        cmp     #(THEIR_PUSHER_MIN_Y+2)
        bcc     invert_y
        cmp     #(THEIR_PUSHER_MAX_Y-2)
        bcs     invert_y
        jmp     check_dx

invert_y:
        ; Invert X
        lda     their_pusher_dy
        NEG_A
        sta     their_pusher_dy

check_dx:
        lda     their_pusher_x
        cmp     #(THEIR_PUSHER_MIN_X+2)
        bcc     invert_x
        cmp     #(THEIR_PUSHER_MAX_X-2)
        bcs     invert_x
        rts

invert_x:
        ; Invert X
        lda     their_pusher_dx
        NEG_A
        sta     their_pusher_dx
        rts
.endproc

.proc init_patrol_delta
        lda     #1
        sta     their_pusher_dx
        sta     their_pusher_dy

        lda     their_pusher_x
        cmp     #(THEIR_PUSHER_MIN_X+2)
        bcs     :+
        lda     #(THEIR_PUSHER_MIN_X+4)
        sta     their_pusher_x
:       cmp     #(THEIR_PUSHER_MAX_X-4)
        bcc     :+
        lda     #(THEIR_PUSHER_MAX_X-2)
        sta     their_pusher_x

:
        lda     their_pusher_y
        cmp     #(THEIR_PUSHER_MIN_Y+12)
        bcs     :+
        lda     #(THEIR_PUSHER_MIN_Y+14)
        sta     their_pusher_y
:       cmp     #(THEIR_PUSHER_MAX_Y-14)
        bcc     :+
        lda     #(THEIR_PUSHER_MAX_Y-12)
        sta     their_pusher_y

:
        rts
.endproc

.proc init_opponent_service
        ldy     #0
        jsr     _play_serve_g_left                                   ; CHANGE A
        ldy     #0
        jsr     _play_serve_g_right                                  ; CHANGE A
        ldy     #0
        jsr     _play_serve_g_right                                  ; CHANGE A
; -------
; End of opponent letter references
; -------

        ; Init serve parameters
        ldy     #0
        sty     their_pusher_dx
        sty     their_pusher_dy

        lda     #0
        sta     req_puck_dx
        lda     #<10
        sta     req_puck_dy

        lda     #(PUCK_INI_X-(my_pusher0_WIDTH/2)+(puck0_WIDTH/2))
        sta     their_pusher_x
        lda     #(THEIR_PUCK_INI_Y-2)
        sta     their_pusher_y

        ; Shorten wait
        jsr     _rand
        cmp     #2
        bcc     :+
        sta     serving

:       lda     #1
        sta     preparing_service
        rts
.endproc

.proc prepare_opponent_service
        lda     preparing_service
        cmp     #1
        beq     go_backwards
        cmp     #2
        beq     suspense
        cmp     #3
        beq     prepare_to_hit
        rts                       ; All's ready

go_backwards:
        lda     their_pusher_y
        sec
        sbc     req_puck_dy
        bmi     preparation_done
        cmp     #THEIR_PUSHER_MIN_Y
        bcc     preparation_done
        tay     ; Don't store yet, must check X isn't out of bounds

        lda     their_pusher_x
        clc
        adc     req_puck_dx
        cmp     #THEIR_PUSHER_MAX_X
        bcs     preparation_done
        cmp     #THEIR_PUSHER_MIN_X
        bcc     preparation_done
        sta     their_pusher_x
        sty     their_pusher_y
        rts

prepare_to_hit:
        lda     req_puck_dx
        sta     their_pusher_dx

        lda     req_puck_dy
        sta     their_pusher_dy
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

their_max_dx:     .byte 8
their_max_dy:     .byte 4
mid_pusher_x:     .byte 1
found_x:          .byte 0
req_puck_dx:      .byte 0
req_puck_dy:      .byte 0
preparing_service:.byte 0
magic_band_rnd:   .byte 0
