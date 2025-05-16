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
; Serial - play with a friend

        .import     my_pusher_x, my_pusher_y
        .import     their_pusher_x, their_pusher_y
        .import     their_pusher_dx, their_pusher_dy
        .import     their_currently_hitting, turn
        .import     puck_x, puck_right_x, puck_y, puck_dx, puck_dy, serving
        .import     game_cancelled
        .import     _rand
        .import     _last_key, _read_key
        .import     _init_puck_position, _set_puck_position
        .import     skip_their_hit_check

        .import     _acia_open, _acia_close
        .import     _acia_get, _acia_put

        .import     _z8530_open, _z8530_close
        .import     _z8530_get, _z8530_put

        .import     _text_mono40, _hgr_force_mono40

        .import     _cout, _strout, _gotoxy, _gotox, _gotoy, _home, _numout

        .import     _big_draw_sprite_s                              ; CHANGE A
        .import     _big_draw_name_s                                ; CHANGE A
        .import     _play_puck_hit

        .import     ostype, _platform_msleep

        .import     return0

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

        .include    "ser-kernel.inc"
        .include    "ser-error.inc"

.segment "s"                                                        ; CHANGE A

.assert * = __OPPONENT_START__+OPPONENT::SPRITE, error ; Make sure the callback is where we think
        jmp     _big_draw_sprite_s                                      ; CHANGE A

.assert * = __OPPONENT_START__+OPPONENT::NAME, error ; Make sure the callback is where we think
        jmp     _big_draw_name_s                                        ; CHANGE A

.assert * = __OPPONENT_START__+OPPONENT::LOSE_POINT, error ; Make sure the callback is where we think
        jmp     return0

.assert * = __OPPONENT_START__+OPPONENT::LOSE_POINT_SND, error ; Make sure the callback is where we think
        jmp     return0

.assert * = __OPPONENT_START__+OPPONENT::WIN_POINT, error ; Make sure the callback is where we think
        jmp     return0

.assert * = __OPPONENT_START__+OPPONENT::WIN_POINT_SND, error ; Make sure the callback is where we think
        jmp     return0

.assert * = __OPPONENT_START__+OPPONENT::END_GAME, error ; Make sure the callback is where we think
        jmp     return0

.assert * = __OPPONENT_START__+OPPONENT::HIT_CB, error
        jmp     hit_cb

.assert * = __OPPONENT_START__+OPPONENT::THINK_CB, error ; Make sure the callback is where we think
.proc _opponent_think
        lda     connected
        bne     :+
        jsr     configure_serial

:       lda     game_cancelled
        bne     error

        jsr     exchange_pusher_coords
        rts

error:
        inc     game_cancelled
        rts
.endproc

.proc pack_pusher_coords
        txa                       ; Keep only 4 most significant bits for X
        and     #%11110000
        sta     tmp1
        tya                       ; Divide Y to fit in 3 bits
        lsr                       ; Only 2 shifts needed because
        lsr                       ; of Y range
        and     #%00001110        ; Mask out lsb
        ora     tmp1
        rts
.endproc

.proc unpack_pusher_coords
        tay                       ; Backup for Y
        and     #%11110000        ; Get X
        tax
        tya
        and     #%00001110        ; Get Y
        asl                       ; Shift it back into place
        asl
        tay
        rts
.endproc

.proc mirror_pusher_x
        sta     tmp1
        lda     #MY_PUSHER_MAX_X
        sec
        sbc     tmp1
        rts
.endproc

.proc mirror_pusher_y
        sta     tmp1
        lda     #MY_PUSHER_MAX_Y
        sec
        sbc     tmp1
        clc
        adc     #THEIR_PUSHER_MIN_Y
        rts
.endproc

.proc mirror_puck_x
        sta     tmp1
        lda     #PUCK_MAX_X
        sec
        sbc     tmp1
        rts
.endproc

.proc mirror_puck_y
        sta     tmp1
        lda     #MY_PUSHER_MAX_Y
        sec
        sbc     tmp1
        clc
        adc     #THEIR_PUSHER_MIN_Y
        rts
.endproc

.proc exchange_pusher_coords
        ; Pusher coords: XXXXYYYk
        ; 4 most-significant bits for X (range 0-224) are kept
        ; 3 bits for Y (range 6-46)
        ; One bit for control
        lda     my_pusher_x
        jsr     mirror_pusher_x
        tax
        lda     my_pusher_y
        jsr     mirror_pusher_y
        tay
        jsr     pack_pusher_coords
        jsr     exchange_char
        jsr     unpack_pusher_coords
        stx     their_pusher_x
        sty     their_pusher_y
        rts
.endproc

.proc send_puck_params
        lda     puck_dx
        sta     prev_puck_dx
        lda     puck_dy
        sta     prev_puck_dy

        php                       ; Timing sensitive, no pesky mouse interrupts
        sei

        lda     #'H'              ; Tell we did hit
        jsr     exchange_char
        ; We don't care the reply, it can't be 'H'
send_puck_params:
        lda     puck_dx          ; Send reverted puck_dx
        NEG_A
        jsr     exchange_char
        ; Ignore the reply
        lda     puck_dy          ; Send reverted puck_dy
        NEG_A
        jsr     exchange_char
        ; Ignore the reply

        lda     puck_x          ; Send mirrored puck_x
        jsr     mirror_puck_x
        jsr     exchange_char
        ; Ignore the reply

        lda     puck_y          ; Send mirrored puck_y
        jsr     mirror_puck_y
        jsr     exchange_char
        ; Ignore the reply

        plp

        ; Set our precise pos from what we sent
        ldx     puck_x
        ldy     puck_y
        jsr     _set_puck_position
        clc
        rts
.endproc

.proc get_puck_params
        ; They hit, get params
        lda     #'?'              ; Get puck_dx
        jsr     exchange_char
        sta     puck_dx
        sta     prev_puck_dx
        lda     #'?'              ; Get puck_dy
        jsr     exchange_char
        sta     puck_dy
        sta     prev_puck_dy

        lda     #'?'              ; Get puck_x
        jsr     exchange_char
        sta     puck_x
        lda     #'?'              ; Get puck_y
        jsr     exchange_char
        tay
        ldx     puck_x
        jsr     _set_puck_position

        ldy     #4                ; Play their hit sound
        jmp     _play_puck_hit
.endproc

.proc hit_cb
        lda     game_cancelled
        beq     check_miss
        rts

check_miss:
        bcc     check_puck_params ; carry clear => normal, set => we missed

        lda     #'M'              ; Tell we did miss
        jmp     exchange_char

check_puck_params:
        lda     prev_puck_dx
        cmp     puck_dx
        bne     update_my_puck_params
        lda     prev_puck_dy
        cmp     puck_dy
        beq     no_puck_params_change

update_my_puck_params:
        jsr     send_puck_params
        rts

no_puck_params_change:

        php                       ; Timing sensitive, no pesky mouse interrupts
        sei

        lda     #'N'              ; Tell we didn't hit
        jsr     exchange_char
        cmp     #'M'
        beq     they_missed       ; They missed
        cmp     #'H'
        beq     update_their_puck_params
        cmp     #'N'
        beq     out               ; They didn't hit either

update_their_puck_params:
        jsr     get_puck_params

out:    plp
        clc
        rts

they_missed:
        plp
        sec
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
        jsr     _strout

get_parameter:
        lda     $FFFF
        sta     tmp_param
        ldx     #0
        jsr     _numout

        lda     #' '
        jsr     _cout

        ldx     tmp_param
        jsr     _read_key
        cmp     #$08
        beq     dec_param
        cmp     #$15
        beq     inc_param
        cmp     #$0D              ; Enter
        beq     done
        cmp     #CH_ESC
        beq     cancel
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

cancel:
        inc     game_cancelled
done:
        rts
.endproc

.proc configure_serial
        jsr     _home
        jsr     _text_mono40

        jsr     setup_defaults

ask_slot:
        lda     #13
        jsr     pusha
        lda     #0
        sta     _last_key         ; Reset last key to avoid pausing on config exit
        jsr     _gotoxy
        lda     #<configure_str
        ldx     #>configure_str
        jsr     _strout

        lda     #0
        jsr     pusha
        lda     #23
        jsr     _gotoxy
        lda     #<help_str
        ldx     #>help_str
        jsr     _strout

        lda     #3
        jsr     _gotoy

        lda     #1+1
        ldx     #7
        jsr     pushax
        lda     #<serial_slot
        ldx     #>serial_slot
        jsr     pushax
        lda     #<slot_str
        ldx     #>slot_str
        jsr     configure_parameter

        lda     game_cancelled
        bne     finish_game

        lda     serial_slot
        ldx     #SER_BAUD_115200
        jsr     open_serial_slot
        cmp     #SER_ERR_OK
        bne     open_error

        lda     #12
        jsr     pusha
        lda     #15
        jsr     _gotoxy
        lda     #<wait_str
        ldx     #>wait_str
        jsr     _strout

        ; Wait for other player
        jsr     wait_connection
        lda     game_cancelled
        bne     finish_game

        jsr     _home
        jmp     _hgr_force_mono40

open_error:
        lda     #12
        jsr     pusha
        lda     #15
        jsr     _gotoxy
        lda     #<open_error_str
        ldx     #>open_error_str
        jsr     _strout
        lda     #<1000
        ldx     #>1000
        jsr     _platform_msleep
        jsr     _home
        jmp     ask_slot
.endproc

.proc finish_game
        jsr     close_serial_slot
        jsr     _home
        jmp     _hgr_force_mono40
.endproc

; Returns with carry set if connection failed.
.proc wait_connection
        ; Flush
:       jsr     serial_wait_and_get
        bcc     :-

        lda     #10
        jsr     pusha
        lda     #16
        jsr     _gotoxy
        lda     #<enter_str
        ldx     #>enter_str
        jsr     _strout

:       lda     KBD
        bpl     :-
        bit     KBDSTRB

        ldy     #$00
        clc
next_char:
        lda     ident_str,y
        beq     out_done            ; All chars sent/received

        jsr     exchange_char
        cmp     ident_str,y         ; Is it the same char?
        bne     out_err             ; No, try again

        lda     game_cancelled      ; Did we esc during exchange?
        bne     out_err

        iny
        bne     next_char

out_done:
        lda     #1
        sta     skip_their_hit_check

        jsr     setup_players
        inc     connected
        rts

out_err:
        inc     game_cancelled
        rts
.endproc

.proc setup_players
        jsr     _rand
        and     #1
        sta     player
        jsr     exchange_char
        cmp     player
        beq     setup_players
        lda     player
        sta     turn
        jmp     _init_puck_position
.endproc

.proc setup_defaults
        bit     ostype
        bpl     :+

        ; Patch defaults and callbacks for
        ; IIgs z8530 integrated serial ports
        lda     #0
        sta     serial_slot

        lda     #<_z8530_open
        ldx     #>_z8530_open
        sta     open_serial_slot+1
        stx     open_serial_slot+2

        lda     #<_z8530_close
        ldx     #>_z8530_close
        sta     close_serial_slot+1
        stx     close_serial_slot+2

        lda     #<_z8530_put
        ldx     #>_z8530_put
        sta     serial_put_char+1
        stx     serial_put_char+2

        lda     #<_z8530_get
        ldx     #>_z8530_get
        sta     serial_get_char+1
        stx     serial_get_char+2

:       rts
.endproc

.proc open_serial_slot
        jmp     _acia_open
.endproc

.proc close_serial_slot
        jmp     _acia_close
.endproc

; Wait a bit and send char.
.proc serial_put
        ldx     #25
:       dex
        bne     :-
.endproc
; Send character in A over serial. Destroys X.
; Does not touch Y.
.proc serial_put_char
        jmp     _acia_put
.endproc

; Returns with char in A and carry clear if
; character available. Destroys X.
; Does not touch Y.
.proc serial_get_char
        jmp     _acia_get
.endproc

; Try 10 times to get a char over serial.
; Destroys X, does not touch Y.
.proc serial_wait_and_get
        lda     #10
        sta     ser_timer

try:    jsr     serial_get_char
        bcc     out

        jsr     check_escape
        bcs     out

        dec     ser_timer
        bne     try
        sec

out:    rts
.endproc

; Sends A over serial, receives A over serial
.proc exchange_char
        php                         ; Timing sensitive, no pesky mouse interrupts
        sei
        jsr     serial_put          ; Send char
        jsr     serial_force_get    ; Get remote char (or escape)
        bcc     :+                  ; Escape?
        inc     game_cancelled      ; We're done
:       plp
        rts
.endproc

.proc check_escape
        lda     KBD                 ; Check for user abort
        bpl     :+
        bit     KBDSTRB
        cmp     #CH_ESC|$80
        beq     out_esc
:       clc
        rts
out_esc:
        sec
        rts
.endproc

.proc serial_force_get
        lda     game_cancelled
        bne     out
        jsr     check_escape
        bcs     out
        jsr     serial_get_char
        bcs     serial_force_get
out:    rts
.endproc

ser_timer:        .word 0
serial_slot:      .byte 2
connected:        .byte 0
player:           .byte 0

their_max_dx:     .byte 8
their_max_dy:     .byte 8
their_max_hit_dy: .byte 10
tmp_param:        .byte 0

prev_puck_dx:     .byte 0
prev_puck_dy:     .byte 0

ident_str:        .asciiz "SHFL1"

wait_str:         .asciiz "WAITING FOR PLAYER"
enter_str:        .asciiz "PRESS ENTER WHEN READY"
configure_str:    .asciiz "CONFIGURE SERIAL"

slot_str:         .asciiz "SLOT: "

open_error_str:   .asciiz "SERIAL OPEN ERROR"

help_str:         .asciiz "ARROW KEYS TO CHANGE, ENTER TO VALIDATE"
