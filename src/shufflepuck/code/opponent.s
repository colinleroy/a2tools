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

        .export     _opponent_think

        .import     their_pusher_x, their_pusher_y
        .import     their_pusher_dx, their_pusher_dy
        .import     puck_x, puck_y, serving

        .importzp   tmp1

        .include    "sprite.inc"
        .include    "their_pusher_coords.inc"
        .include    "puck_coords.inc"
        .include    "constants.inc"

THEIR_MAX_DX       = 4
THEIR_MAX_DY       = 8

.proc _opponent_think
        lda     serving
        beq     catch

        ; We're serving
        inc     serving
        beq     hit               ; After a while we got to serve

        lda     their_pusher_y
        cmp     #(THEIR_PUSHER_MIN_Y+1)
        bcs     :+
        lda     #$01
        sta     their_pusher_dy
        rts
:       cmp     #(THEIR_PUCK_INI_Y-2)
        bcs     :+
        rts
:       lda     #$FF
        sta     their_pusher_dy
        rts

catch:
        lda     their_pusher_x
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

        ; Update DY now
        lda     puck_y
        cmp     #MY_PUSHER_MAX_Y   ; Move forward as long as puck isn't in the end of the player zone (defensive)
        ;cmp     #MY_PUSHER_MIN_Y   ; Move forward as long as puck isn't in the player zone (a bit dangerous)
        ;cmp     #MID_BOARD         ; Move forward as long as puck isn't in the player half of the board (dangerous)
        ;cmp     #THEIR_PUSHER_MAX_Y; Move forward as long as puck isn't in our zone (very dangerous)
        bcs     move_forwards_slow
        cmp     #THEIR_PUSHER_MAX_Y
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

.bss
serve_dy:       .res 1
