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

        .import     their_pusher_x, their_pusher_y
        .import     their_pusher_dx, their_pusher_dy
        .import     puck_x, puck_y, puck_dy, serving
        .import     _big_draw_sprite_a                              ; CHANGE A
        .import     _big_draw_name_a                                ; CHANGE A

        .import     __OPPONENT_START__
        .importzp   tmp1

        .include    "my_pusher0.gen.inc"
        .include    "puck0.gen.inc"
        .include    "../code/sprite.inc"
        .include    "../code/their_pusher_coords.inc"
        .include    "../code/puck_coords.inc"
        .include    "../code/constants.inc"
        .include    "../code/opponent_file.inc"

THEIR_MAX_DX       = 4
THEIR_MAX_DY       = 8

.segment "a"                                                        ; CHANGE A

.assert * = __OPPONENT_START__+OPPONENT::SPRITE, error ; Make sure the callback is where we think
sprite:
      jmp _big_draw_sprite_a                                      ; CHANGE A

.assert * = __OPPONENT_START__+OPPONENT::NAME, error ; Make sure the callback is where we think
name:
      jmp _big_draw_name_a                                        ; CHANGE A

.assert * = __OPPONENT_START__+OPPONENT::THINK_CB, error ; Make sure the callback is where we think
.proc _opponent_think
        lda     serving
        beq     serve_or_catch

prepare_service:
        cmp     #$01
        bne     :+

        ; Who serves?
        lda     puck_y
        cmp     #THEIR_PUCK_INI_Y
        bne     serve_or_catch    ; It's the player

        ; Init serve parameters
        lda     #(ABS_MAX_DX)
        sta     their_pusher_dx

:       ; We're serving
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
        lda     their_pusher_x
        clc                       ; Center puck on pusher
        adc     #((my_pusher0_WIDTH-puck0_WIDTH)/2)
        cmp     puck_x
        bcs     move_left

move_right:
        lda     puck_x
        sec
        sbc     their_pusher_x

        cmp     #THEIR_MAX_DX
        bcc     store_dx
        lda     #THEIR_MAX_DX
        clc
        jmp     store_dx

move_left:
        lda     their_pusher_x
        sec
        sbc     puck_x

        cmp     #THEIR_MAX_DX
        bcc     :+
        lda     #THEIR_MAX_DX

:       clc
        eor     #$FF
        adc     #$01
store_dx:
        sta     their_pusher_dx

        ; Update DY now if we can
        lda     puck_dy           ; Is the puck moving?
        bne     :+

        lda     puck_y            ; Is it possible to hit the puck if we move forward now?
        sec
        sbc     their_pusher_y
        cmp     their_pusher_dx
        bcs     :+
        rts                       ; No, so return

:       lda     puck_y
        cmp     #MY_PUSHER_MAX_Y   ; Move forward as long as puck isn't in the end of the player zone (defensive)
        ;cmp     #MY_PUSHER_MIN_Y   ; Move forward as long as puck isn't in the player zone (a bit dangerous)
        ;cmp     #MID_BOARD         ; Move forward as long as puck isn't in the player half of the board (dangerous)
        ;cmp     #THEIR_PUSHER_MAX_Y; Move forward as long as puck isn't in our zone (very dangerous)
        bcs     move_forwards_slow
        cmp     #(THEIR_PUSHER_MAX_Y)
        bcs     move_backwards
        ; Otherwise prepare to hit
hit:
        lda     #(ABS_MAX_DY)
        sta     their_pusher_dy
        rts

move_forwards_slow:
        lda     #(ABS_MAX_DY/8)
        sta     their_pusher_dy
        rts

move_backwards:
        lda     #(ABS_MAX_DY/4)
        clc
        eor     #$FF
        adc     #$01
        sta     their_pusher_dy
        rts
.endproc

.proc invert_pusher_dx
        lda     their_pusher_dx
        clc
        eor     #$FF
        adc     #$01
        sta     their_pusher_dx
        rts
.endproc


.bss
serve_dy:       .res 1
