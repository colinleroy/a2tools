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
; Vinnie
; How to beat: Aim for the other side of the table

        .import     their_pusher_x, their_pusher_y
        .import     their_pusher_dx, their_pusher_dy
        .import     their_currently_hitting
        .import     puck_x, puck_right_x, puck_y, puck_dx, puck_dy, serving, their_score
        .import     _guess_puck_x_at_y
        .import     _rand

        .import     _big_draw_sprite_c                              ; CHANGE A
        .import     _big_draw_name_c                                ; CHANGE A
        .import     _big_draw_lose_c                                ; CHANGE A
        .import     _big_draw_win_c                                 ; CHANGE A
        .import     _play_lose_c                                    ; CHANGE A

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

THEIR_MAX_DX = 2
.segment "c"                                                        ; CHANGE A

.assert * = __OPPONENT_START__+OPPONENT::SPRITE, error ; Make sure the callback is where we think
        jmp     _big_draw_sprite_c                                      ; CHANGE A

.assert * = __OPPONENT_START__+OPPONENT::NAME, error ; Make sure the callback is where we think
        jmp     _big_draw_name_c                                        ; CHANGE A

.assert * = __OPPONENT_START__+OPPONENT::LOSE_POINT, error ; Make sure the callback is where we think
        jmp     animate_lose

.assert * = __OPPONENT_START__+OPPONENT::LOSE_POINT_SND, error ; Make sure the callback is where we think
        jmp     sound_lose

.assert * = __OPPONENT_START__+OPPONENT::WIN_POINT, error ; Make sure the callback is where we think
        jmp     animate_win

.assert * = __OPPONENT_START__+OPPONENT::WIN_POINT_SND, error ; Make sure the callback is where we think
        jmp     return0

.assert * = __OPPONENT_START__+OPPONENT::END_GAME, error ; Make sure the callback is where we think
        jmp     return0

.assert * = __OPPONENT_START__+OPPONENT::THINK_CB, error ; Make sure the callback is where we think
.proc _opponent_think
        lda     serving
        beq     serve_or_catch

init_service:
        cmp     #$01
        bne     prepare_service

        lda     #$00
        sta     found_x
        sta     no_fast

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

        lda     #$01
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
        bne     catch

        lda     #$00              ; Cancel Y speed
        sta     their_pusher_dy

catch:

        ; Is it time to figure out destination X?
        lda     puck_dy
        bpl     move
        lda     puck_y
        cmp     #MID_BOARD
        bcs     move

        ; Figure X if not already done
        lda     found_x
        bne     move
        lda     #25
        jsr     _guess_puck_x_at_y

        ; And store it (but not 0, so we know we found it)
        tax
        bne     :+
        inx
:       stx     found_x

move:   
        lda     found_x
        beq     follow_puck

        ; Is the puck destination too far ?
        lda     their_pusher_x
        sec
        sbc     found_x
        bmi     puck_right_of_pusher
puck_left_of_pusher:
        cmp     #82+puck0_WIDTH
        bcc     close_enough
        bcs     do_no_fast
puck_right_of_pusher:
        NEG_A
        cmp     #82+my_pusher0_WIDTH
        bcc     close_enough

do_no_fast:
        lda     #1
        sta     no_fast
        jmp     follow_puck

close_enough:
        lda     no_fast
        bne     follow_puck
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
        
        lda     #$00
        jmp     store_dx

follow_puck:
        GET_DELTA_TO_PUCK
        ; Bind to max dx
        BIND_SIGNED #THEIR_MAX_DX
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
        ; Forget found X
        lda     #0
        sta     found_x
        sta     no_fast

        ; Get a 4-11 DY
        jsr     _rand
        lsr
        lsr
        lsr
        lsr
        lsr
        clc
        adc     #4
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

.proc animate_lose
        ldx     #((42+98)/7)    ; left X of sprite change + left X of big sprite
        ldy     #(52)           ; bottom Y of sprite change
        jmp     _big_draw_lose_c                                        ; CHANGE A
.endproc

.proc animate_win
        ldx     #((35+98)/7)    ; left X of sprite change + left X of big sprite
        ldy     #(76)           ; bottom Y of sprite change
        jmp     _big_draw_win_c                                        ; CHANGE A
.endproc

.proc sound_lose
        ldy     #0
        jmp     _play_lose_c                                            ; CHANGE A
.endproc

found_x:          .byte 0
no_fast:          .byte 0
