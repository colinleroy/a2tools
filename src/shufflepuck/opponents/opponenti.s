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
; SKIP
; How to beat: Just send the puck in his general direction

        .import     their_pusher_x, their_pusher_y
        .import     their_pusher_dx, their_pusher_dy
        .import     their_currently_hitting
        .import     puck_x, puck_right_x, puck_y, puck_dy, serving, their_score
        .import     _rand
        .import     _last_key

        .import     _init_text, _init_hgr

        .import     _cputc, _cputs, _revers, _gotoxy, _gotox, _gotoy, _clrscr, _cutoa

        .import     _big_draw_sprite_i                              ; CHANGE A
        .import     _big_draw_name_i                                ; CHANGE A
        .import     _big_draw_normal_i                              ; CHANGE A
        .import     _big_draw_moving_i_1                            ; CHANGE A
        .import     _big_draw_moving_i_2                            ; CHANGE A

        .import     _play_lose_i_1, _play_lose_i_2
        .import     _play_win_i_1, _play_win_i_2, _play_win_i_3, _play_win_i_4
        .import     _play_serve_i
        .import     _platform_msleep

        .import     __OPPONENT_START__

        .import     pusha, pushax, popax
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

.segment "i"                                                        ; CHANGE A

.assert * = __OPPONENT_START__+OPPONENT::SPRITE, error ; Make sure the callback is where we think
sprite:
        ldx     #(98/7)
        ldy     #76
        jmp     _big_draw_sprite_i                                      ; CHANGE A

.assert * = __OPPONENT_START__+OPPONENT::NAME, error ; Make sure the callback is where we think
name:
        ldx     #(7/7)
        ldy     #39
        jmp     _big_draw_name_i                                        ; CHANGE A

.assert * = __OPPONENT_START__+OPPONENT::LOSE_POINT, error ; Make sure the callback is where we think
lose_animation:
        rts
        .res    6

.assert * = __OPPONENT_START__+OPPONENT::LOSE_POINT_SND, error ; Make sure the callback is where we think
lose_sound:
        jmp     lose_snd
        .res 2

.assert * = __OPPONENT_START__+OPPONENT::WIN_POINT, error ; Make sure the callback is where we think
win_animation:
        rts
        .res    6

.assert * = __OPPONENT_START__+OPPONENT::WIN_POINT_SND, error ; Make sure the callback is where we think
win_sound:
        jmp     win_snd
        .res 2

.assert * = __OPPONENT_START__+OPPONENT::THINK_CB, error ; Make sure the callback is where we think
.proc _opponent_think
        lda     _last_key
        cmp     #' '
        bne     :+
        jmp     configure_dc3

:       lda     serving
        beq     catch

init_service:
        cmp     #$01
        bne     prepare_service

        ; Who serves?
        lda     puck_y
        cmp     #THEIR_PUCK_INI_Y
        bne     catch    ; It's the player

        ; Init serve parameters
        lda     #0
        sta     their_pusher_dx

        ; Shorten wait
        lda     #246
        sta     serving

prepare_service:
        ; We're serving
        inc     serving
        beq     serve       ; After a while we got to serve

        lda     #$01
        sta     their_pusher_dy
        ldy     #0
        jsr     _play_serve_i
        rts

serve:
catch:
        lda     puck_dy           ; Is the puck moving?
        bne     :+

        lda     #$00              ; Cancel Y speed
        sta     their_pusher_dy

:       GET_DELTA_TO_PUCK
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
        lda     their_max_hit_dy
        sta     their_pusher_dy

        ; Get a 0-15 DX
        jsr     _rand
        lsr
        lsr
        lsr
        lsr
        sec
        ; Make it -8/7
        sbc     #8
        sta     their_pusher_dx
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

.proc configure_parameter
        sta     str_low+1
        stx     str_high+1
        ; parameter
        jsr     popax
        sta     get_parameter+1
        sta     update_parameter+1
        stx     get_parameter+2
        stx     update_parameter+2
        ; lower bound
        jsr     popax
        sta     dec_param+1
        stx     inc_param+1

print:
        lda     #0
        jsr     _gotox

str_low:
        lda     #$FF
str_high:
        ldx     #$FF
        jsr     _cputs

get_parameter:
        lda     $FFFF
        sta     tmp_param
        ldx     #0
        jsr     _cutoa

        lda     #' '
        jsr     _cputc

        ldx     tmp_param
:       lda     KBD
        bpl     :-
        bit     KBDSTRB
        and     #$7F
        cmp     #$08
        beq     dec_param
        cmp     #$15
        beq     inc_param
        cmp     #$0D
        beq     done
        bne     print
dec_param:
        cpx     #$00
        bcc     print
        dex
        jmp     update_parameter
inc_param:
        cpx     #$FF
        bcs     print
        inx
update_parameter:
        stx     $FFFF
        jmp     print
done:
        rts
.endproc

.proc configure_dc3
        jsr     _clrscr
        jsr     _init_text

        lda     #13
        jsr     pusha
        lda     #0
        jsr     _gotoxy
        lda     #<configure_str
        ldx     #>configure_str
        jsr     _cputs

        lda     #0
        jsr     pusha
        lda     #23
        jsr     _gotoxy
        lda     #<help_str
        ldx     #>help_str
        jsr     _cputs

        lda     #3
        jsr     _gotoy

        lda     #5
        ldx     #ABS_MAX_DX
        jsr     pushax
        lda     #<their_max_dx
        ldx     #>their_max_dx
        jsr     pushax
        lda     #<max_delta_str
        ldx     #>max_delta_str
        jsr     configure_parameter

        lda     their_max_dx
        sta     their_max_dy

        lda     #5
        jsr     _gotoy

        lda     #5
        ldx     #ABS_MAX_DX
        jsr     pushax
        lda     #<their_max_hit_dy
        ldx     #>their_max_hit_dy
        jsr     pushax
        lda     #<max_hit_str
        ldx     #>max_hit_str
        jsr     configure_parameter

        jsr     _clrscr
        lda     #1
        jmp     _init_hgr
.endproc

; Sequence is A/B/A/normal
.proc animate_a
        ldx     #((35+98)/7)
        ldy     #25
        jmp     _big_draw_moving_i_1                                    ; CHANGE A
.endproc

.proc animate_b
        ldx     #((35+98)/7)
        ldy     #25
        jmp     _big_draw_moving_i_2                                    ; CHANGE A
.endproc

.proc animate_normal
        ldx     #((35+98)/7)
        ldy     #25
        jmp     _big_draw_normal_i                                      ; CHANGE A
.endproc

.proc win_snd
        jsr     animate_a
        ldy     #0
        jsr     _play_win_i_1
        lda     #50
        ldx     #0
        jsr     _platform_msleep
        jsr     animate_b
        ldy     #0
        jsr     _play_win_i_2
        lda     #50
        ldx     #0
        jsr     _platform_msleep
        jsr     animate_a
        ldy     #0
        jsr     _play_win_i_3
        lda     #5
        ldx     #0
        jsr     _platform_msleep
        jsr     animate_normal
        ldy     #0
        jmp     _play_win_i_4
.endproc

.proc lose_snd
        jsr     animate_a
        ldy     #0
        jsr     _play_lose_i_1
        jsr     animate_b
        ldy     #0
        jsr     _play_lose_i_2
        jsr     animate_a
        ldy     #0
        jsr     _play_lose_i_2
        jmp     animate_normal
.endproc

their_max_dx:     .byte 8
their_max_dy:     .byte 8
their_max_hit_dy: .byte 10
tmp_param:        .byte 0

configure_str:    .asciiz "CONFIGURE DC3"

max_delta_str:    .asciiz "MAX MOVE SPEED: "
max_hit_str:      .asciiz "HIT FORCE:      "

help_str:         .asciiz "ARROW KEYS TO CHANGE, ENTER TO VALIDATE"
