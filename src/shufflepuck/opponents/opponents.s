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
        .import     player_puck_delta_change
        .import     game_cancelled
        .import     _rand
        .import     _last_key, _read_key
        .import     _init_puck_position, _set_puck_position
        .import     their_hit_check_via_serial
        .import     _set_max_puck_delta

        .import     _puck_reinit_my_order, _puck_reinit_their_order

        .import     _text_mono40, _hgr_mixon, _hgr_mixoff
        .import     _hgr_unset_mono40, _hgr_force_mono40
        .import     _get_iigs_speed, _set_iigs_speed

        .import     _serial_open, _serial_close
        .import     _serial_putc_direct, _serial_read_byte_direct

        .import     _cout, _strout, _gotoxy, _gotox, _gotoy, _home, _numout

        .import     _big_draw_sprite_s_1    ; Susan
        .import     _big_draw_sprite_s_2    ; Steve
        .import     _big_draw_sprite_s_3    ; Mx Raccoon
        .import     _big_draw_sprite_s_4, _big_draw_lose_s_4, _big_draw_win_s_4    ; Calvin
        .import     _big_draw_sprite_s_5, _big_draw_lose_s_5, _big_draw_win_s_5    ; Mafalda
        .import     _big_draw_sprite_s_6, _big_draw_lose_s_6, _big_draw_win_s_6    ; Luigi
        .import     _big_draw_sprite_s_7    ; Smudge
        .import     _big_draw_name_s
        .import     _update_opponent
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
        .include    "accelerator.inc"

IO_BARRIER = $FF

.macro DBG arg
        ;sta     arg
.endmacro

.segment "s"                                                        ; CHANGE A

.assert * = __OPPONENT_START__+OPPONENT::SPRITE, error ; Make sure the callback is where we think
        jmp     _big_draw_sprite_s_3                                    ; CHANGE A

.assert * = __OPPONENT_START__+OPPONENT::NAME, error ; Make sure the callback is where we think
        jmp     _big_draw_name_s                                        ; CHANGE A

.assert * = __OPPONENT_START__+OPPONENT::LOSE_POINT, error ; Make sure the callback is where we think
        jmp     avatar_set_lose_sprite

.assert * = __OPPONENT_START__+OPPONENT::LOSE_POINT_SND, error ; Make sure the callback is where we think
        jmp     return0

.assert * = __OPPONENT_START__+OPPONENT::WIN_POINT, error ; Make sure the callback is where we think
        jmp     avatar_set_win_sprite

.assert * = __OPPONENT_START__+OPPONENT::WIN_POINT_SND, error ; Make sure the callback is where we think
        jmp     return0

.assert * = __OPPONENT_START__+OPPONENT::END_GAME, error ; Make sure the callback is where we think
        jmp     finish_game

.assert * = __OPPONENT_START__+OPPONENT::HIT_CB, error
        jmp     hit_cb

.assert * = __OPPONENT_START__+OPPONENT::THINK_CB, error ; Make sure the callback is where we think
.proc _opponent_think
        lda     connected
        bne     :+
        jsr     configure_serial

        ; The usual dy limit requires super-human reflexes
        ldx     #ABS_MAX_DX
        ldy     #ABS_MAX_DY/2
        jsr     _set_max_puck_delta

:       lda     game_cancelled
        bne     error

        lda     serving
        beq     out

        ; At service, both players agree on everything, make sure we don't
        ; send a useless HIT message.
        lda     #$00
        sta     player_puck_delta_change

out:    rts

error:
        inc     game_cancelled
        rts
.endproc

.proc pack_pusher_coords
        txa                       ; Keep only 5 most significant bits for X
        and     #%11111000
        sta     tmp1
        tya                       ; Divide Y to fit in 3 bits
        lsr                       ; Only 3 shifts needed because
        lsr                       ; of Y range
        lsr
        ora     tmp1
        rts
.endproc

.proc unpack_pusher_coords
        tay                       ; Backup for Y
        and     #%11111000        ; Get X
        tax
        tya                       ; Restore for Y
        and     #%00000111        ; Get Y
        asl                       ; Shift it back into place
        asl
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
        lda     #PUCK_MAX_X-1
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

.proc send_pusher_coords
        lda     my_pusher_x
        jsr     mirror_pusher_x
        tax
        lda     my_pusher_y
        jsr     mirror_pusher_y
        tay
        jsr     pack_pusher_coords
        jsr     _serial_putc_direct
        rts
.endproc

.proc send_puck_params
        lda     #$00             ; Reset hit indicator
        sta     player_puck_delta_change

        lda     #'H'             ; Tell we did hit
        DBG     $3022
        jsr     _serial_putc_direct
        jsr     serial_force_get
        bcs     cancel
        DBG     $3023
        cmp     #'?'
        beq     send

cancel:
        inc     game_cancelled   ; We should not be there!
        rts

send:
        lda     puck_dx          ; Send reverted puck_dx
        NEG_A
        DBG     $3024
        jsr     _serial_putc_direct
        jsr     serial_force_get
        bcs     cancel
        ; Ignore the reply
        lda     puck_dy          ; Send reverted puck_dy
        NEG_A
        DBG     $3025
        jsr     _serial_putc_direct
        jsr     serial_force_get
        bcs     cancel
        ; Ignore the reply

        lda     puck_x          ; Send mirrored puck_x
        jsr     mirror_puck_x
        DBG     $3026
        jsr     _serial_putc_direct
        jsr     serial_force_get
        bcs     cancel
        ; Ignore the reply

        lda     puck_y          ; Send mirrored puck_y
        jsr     mirror_puck_y
        DBG     $3027
        jsr     _serial_putc_direct
        jsr     serial_force_get
        bcs     cancel
        ; Ignore the reply

        lda     #$00
        DBG     $3022
        DBG     $3023
        DBG     $3024
        DBG     $3025
        DBG     $3026
        DBG     $3027
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
        jsr     _serial_putc_direct
        DBG     $3023
        jsr     serial_force_get
        bcs     cancel
        DBG     $3024
        sta     puck_dx
        lda     #'?'              ; Get puck_dy
        jsr     _serial_putc_direct
        jsr     serial_force_get
        bcs     cancel
        DBG     $3025
        sta     puck_dy

        lda     #'?'              ; Get puck_x
        jsr     _serial_putc_direct
        jsr     serial_force_get
        bcs     cancel
        DBG     $3026
        sta     puck_x
        lda     #'?'              ; Get puck_y
        jsr     _serial_putc_direct
        jsr     serial_force_get
        bcs     cancel
        DBG     $3027
        sta     puck_y
        ldx     puck_x
        ldy     puck_y
        jsr     _set_puck_position
        jsr     _puck_reinit_their_order

        lda     #$00
        DBG     $3023
        DBG     $3024
        DBG     $3025
        DBG     $3026
        DBG     $3027

        lda     their_currently_hitting
        beq     :+
        rts

:       lda     #15
        sta     their_currently_hitting
        ldy     #4                ; Play their hit sound
        jmp     _play_puck_hit

cancel: inc     game_cancelled
        rts
.endproc

.proc hit_cb
        lda     game_cancelled
        beq     prepare_exchange
        rts

prepare_exchange:
        php                       ; Shut down IRQs
        sei

        bcs     must_send_miss    ; If carry set, we missed, we must tell

        ; If we hit the puck, we must send new params
        bit     player_puck_delta_change
        bmi     must_send_puck_params

        beq     send_byte         ; Otherwise we have nothing special to say

must_send_miss:
:       lda     #IO_BARRIER         ; This needs sync, send a barrier
        DBG     $2023
        jsr     _serial_putc_direct
        jsr     serial_force_get    ; And wait for its ack
        bcs     cancel
        cmp     #IO_BARRIER
        bne     :-
        DBG     $2024

        lda     #'M'                ; Send Miss
        jsr     _serial_putc_direct
        DBG     $2025
        jsr     serial_force_get    ; Get ack
        DBG     $2026
        bcs     cancel
        jmp     puck_crashed        ; And get out

must_send_puck_params:
:       lda     #IO_BARRIER         ; This needs sync, send a barrier
        DBG     $2023
        jsr     _serial_putc_direct
        jsr     serial_force_get    ; And wait for its ack
        bcs     cancel
        cmp     #IO_BARRIER
        bne     :-
        DBG     $2024
        jsr     send_puck_params    ; Send puck params
        jmp     clc_out             ; And get out

send_byte:
        jsr     serial_wait_and_get ; Do we have a message,
        bcs     send_coords         ; No, send our coordinates
        DBG     $2023
        cmp     #IO_BARRIER         ; Yes, is it a barrier?
        bne     :+
        jsr     _serial_putc_direct ; Yes, ack it
        DBG     $2024
        jmp     do_read             ; And go read the message

:       jsr     unpack_pusher_coords; We only got coords,
        stx     their_pusher_x      ; Update them
        sty     their_pusher_y

send_coords:
        jsr     send_pusher_coords  ; And send our own coordinates
        plp
        rts

do_read:
        jsr     serial_force_get    ; Get the message
        DBG     $2025
        bcs     cancel
        cmp     #IO_BARRIER         ; We got an extra barrier byte,
        beq     do_read             ; Discard it

        cmp     #'M'
        beq     they_crashed        ; They missed, handle that
        cmp     #'H'
        beq     they_hit            ; They hit the puck, handle it

cancel:
        inc     game_cancelled      ; We should never be there, so bail
        bne     clc_out

they_hit:
        jsr     get_puck_params     ; Get new puck parameters from opponent

clc_out:plp                         ; All good, continue to play
        clc
        bcc     out

they_crashed:
        jsr     _serial_putc_direct ; Send ack
puck_crashed:
        plp
        sec                         ; Caller expects carry set when puck crashed

out:    lda     #$00
        DBG     $2023
        DBG     $2024
        DBG     $2025
        rts
.endproc

.proc configure_slot
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

        bit     ostype
        bmi     print_port_name
print_slot_number:
        jsr     _numout

        lda     #' '
        jsr     _cout
        jmp     update

print_port_name:
        lda     #<modem_str
        ldx     #>modem_str
        ldy     tmp_param
        beq     :+
        lda     #<printer_str
        ldx     #>printer_str
:       jsr     _strout

update:
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

.proc avatar_set_win_sprite
cx:     ldx     #0
cy:     ldy     #0
sprite: jmp     return0
.endproc


.proc avatar_set_lose_sprite
cx:     ldx     #0
cy:     ldy     #0
sprite: jmp     return0
.endproc

; Call with carry set to update the table background
.proc update_avatar
        php
        asl
        tay
        lda     avatar_sprites,y
        iny
        ldx     avatar_sprites,y
        sta     __OPPONENT_START__+OPPONENT::SPRITE+1
        stx     __OPPONENT_START__+OPPONENT::SPRITE+2

        dey                       ; coords pointer
        tya
        asl
        tax                       ; sprite pointer

        lda     avatar_an_coords, y

        sta     avatar_set_win_sprite::cx+1
        sta     avatar_set_lose_sprite::cx+1
        iny
        lda     avatar_an_coords, y
        sta     avatar_set_win_sprite::cy+1
        sta     avatar_set_lose_sprite::cy+1

        lda     avatar_animations,x
        sta     avatar_set_win_sprite::sprite+1
        inx
        lda     avatar_animations,x
        sta     avatar_set_win_sprite::sprite+2
        inx
        lda     avatar_animations,x
        sta     avatar_set_lose_sprite::sprite+1
        inx
        lda     avatar_animations,x
        sta     avatar_set_lose_sprite::sprite+2

        plp
        jmp     _update_opponent
.endproc

.proc configure_avatar
        lda     #0
        jsr     _gotox

        lda     #<avatar_str
        ldx     #>avatar_str
        jsr     _strout

        lda     my_avatar_num
        clc                       ; No need to backup table
        jsr     update_avatar

        lda     my_avatar_num
        asl
        tay
        lda     avatar_names,y
        iny
        ldx     avatar_names,y
        jsr     _strout

update:
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
        bne     configure_avatar
dec_param:
        lda     my_avatar_num
        beq     configure_avatar
        dec     my_avatar_num
        jmp     configure_avatar
inc_param:
        ldx     my_avatar_num
        inx
        cpx     #NUM_AVATARS
        beq     configure_avatar
        inc     my_avatar_num
        jmp     configure_avatar

cancel:
        inc     game_cancelled
done:
        rts
.endproc

.proc configure_serial
        jsr     _home
        jsr     _hgr_mixon
        jsr     _hgr_unset_mono40

        jsr     setup_defaults

ask_slot:
        lda     #0
        sta     _last_key         ; Reset last key to avoid pausing on config exit

        lda     #0
        jsr     pusha
        lda     #23
        jsr     _gotoxy
        lda     #<help_str
        ldx     #>help_str
        jsr     _strout

        lda     #20
        jsr     _gotoy

        bit     ostype
        bmi     iigs
        lda     #1+1
        ldx     #7
        jmp     :+
iigs:
        lda     #0+1
        ldx     #1
:       jsr     pushax
        lda     #<serial_slot
        ldx     #>serial_slot
        jsr     pushax
        lda     #<slot_str
        ldx     #>slot_str
        jsr     configure_slot

        lda     game_cancelled
        bne     finish_game

        lda     serial_slot
        ldx     #SER_BAUD_115200
        jsr     _serial_open
        cmp     #SER_ERR_OK
        bne     open_error

        php                       ; Disable interrupts until connection ready
        sei                       ; The other player's DTR/DSR toggles at init
                                  ; will trigger interrupts even if our ACIA
                                  ; has them disabled, which is Not Good(tm)
                                  ; for ProDOS

        jsr     configure_avatar  ; Let player choose their avatar

        jsr     _hgr_mixoff       ; Turnoff HGR mix, set text,
        jsr     _hgr_force_mono40
        jsr     _home

        lda     #10               ; print WAITING string
        jsr     pusha
        lda     #12
        jsr     _gotoxy
        lda     #<wait_str
        ldx     #>wait_str
        jsr     _strout

        jsr     _text_mono40      ; and show it

        jsr     wait_connection   ; Wait for other player

        plp                       ; We can now re-enable interrupts.

        lda     game_cancelled    ; Did we cancel at some point?
        bne     finish_game

        jsr     _get_iigs_speed   ; Slow down IIgs
        sta     iigs_spd
        lda     #SPEED_SLOW
        jsr     _set_iigs_speed

        jsr     _home
        jmp     _hgr_force_mono40

open_error:
        lda     #12
        jsr     pusha
        lda     #22
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
        jsr     _serial_close

        lda     iigs_spd
        jsr     _set_iigs_speed

        jsr     _home
        jsr     _hgr_mixoff
        jmp     _hgr_force_mono40
.endproc

; Returns with carry set if connection failed.
.proc wait_connection
        ; Ping regularly
:       lda     #100
        ldx     #0
        jsr     _platform_msleep

        lda     game_cancelled
        bne     out_err

        jsr     _serial_putc_direct
        jsr     serial_wait_and_get
        bcs     :-

        ; We got a reply. Stop sending and flush
:       lda     #$FF
        jsr     exchange_char
        cmp     #$FF
        bne     :-

        ; Identify
        lda     #0
        jsr     pusha
        lda     #13
        jsr     _gotoxy
        lda     #<ready_str
        ldx     #>ready_str
        jsr     _strout

        ldy     #$00
        clc

next_char:
        lda     ident_str,y
        beq     out_done            ; All chars sent/received

        jsr     _serial_putc_direct
:       jsr     serial_force_get
        bcs     out_err
        cmp     #$FF
        beq     :-
        cmp     ident_str,y         ; Is it the same char?
        bne     out_err             ; No, try again

        lda     game_cancelled      ; Did we esc during exchange?
        bne     out_err

        iny
        bne     next_char

out_done:
        lda     my_avatar_num       ; Exchange avatars
        jsr     exchange_char
        sta     their_avatar_num
        sec                         ; Update avatar AND backup table
        jsr     update_avatar

        lda     #1
        sta     their_hit_check_via_serial

        jsr     setup_players
        inc     connected
        rts

out_err:
        inc     game_cancelled
        rts
.endproc

.proc setup_players
        jsr     _rand             ; Choose a random player number
        and     #1                ; between 0 and 1
        sta     player            ; Store it.

        jsr     exchange_char     ; Send my number, get theirs
        cmp     player            ; Did they chose the same?
        beq     setup_players     ; Yes, so try again.

        lda     player            ; We agree!
        sta     turn              ; Store whose turn it is.
        jmp     _init_puck_position
.endproc

.proc setup_defaults
        bit     ostype
        bpl     :+

        ; Patch defaults and callbacks for
        ; IIgs z8530 integrated serial ports
        lda     #0
        sta     serial_slot

:       rts
.endproc

; Try 20 times to get a char over serial.
; (20 times to make sure the other side has time to send)
; Destroys X, does not touch Y.
.proc serial_wait_and_get
        lda     #20
        sta     ser_timer

try:    jsr     _serial_read_byte_direct
        bcc     out

        dec     ser_timer
        bne     try

        jsr     check_escape
        bcs     out_abort
        sec

out:    rts
out_abort:
        inc     game_cancelled
        rts
.endproc

; Sends A over serial, receives A over serial
.proc exchange_char
        php
        sei
        jsr     _serial_putc_direct ; Send char
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
:       jsr     _serial_read_byte_direct
        bcc     out
        jsr     check_escape
        bcc     :-
out:    rts
.endproc

ser_timer:        .word   0
serial_slot:      .byte   2
connected:        .byte   0
player:           .byte   0

tmp_param:        .byte   0
my_avatar_num:    .byte   0
their_avatar_num: .byte   0
iigs_spd:         .byte   0
barrier_tries:    .byte   0

ident_str:        .asciiz "SHFL1"

wait_str:         .asciiz "WAITING FOR PLAYER"
configure_str:    .asciiz "CONFIGURE SERIAL"

slot_str:         .asciiz "SERIAL SLOT: "

open_error_str:   .asciiz "SERIAL OPEN ERROR"

help_str:         .asciiz "ARROW KEYS TO CHANGE, ENTER TO VALIDATE"
ready_str:        .asciiz "                 READY!                "

modem_str:        .asciiz "MODEM  "
printer_str:      .asciiz "PRINTER"

avatar_str:       .asciiz "YOUR AVATAR: "

raccoon_str:      .asciiz "MX RACCOON"
calvin_str:       .asciiz "CALVIN    "
mafalda_str:      .asciiz "MAFALDA   "
susan_str:        .asciiz "SUSAN     "
steve_str:        .asciiz "STEVE     "
luigi_str:        .asciiz "LUIGI     "
smudge_str:       .asciiz "SMUDGE    "

avatar_names:     .addr   raccoon_str
                  .addr   calvin_str
                  .addr   mafalda_str
                  .addr   susan_str
                  .addr   steve_str
                  .addr   luigi_str
                  .addr   smudge_str

NUM_AVATARS = (* - avatar_names)/2

avatar_sprites:   .addr   _big_draw_sprite_s_3
                  .addr   _big_draw_sprite_s_4
                  .addr   _big_draw_sprite_s_5
                  .addr   _big_draw_sprite_s_1
                  .addr   _big_draw_sprite_s_2
                  .addr   _big_draw_sprite_s_6
                  .addr   _big_draw_sprite_s_7

NUM_SPRITES = (* - avatar_sprites)/2
.assert NUM_AVATARS = NUM_SPRITES, error

avatar_animations:.addr    return0, return0
                  .addr    _big_draw_win_s_4, _big_draw_lose_s_4
                  .addr    _big_draw_win_s_5, _big_draw_lose_s_5
                  .addr    return0, return0
                  .addr    return0, return0
                  .addr    _big_draw_win_s_6, _big_draw_lose_s_6
                  .addr    return0, return0

NUM_ANIMATIONS = (* - avatar_animations)/4
.assert NUM_ANIMATIONS = NUM_SPRITES, error

avatar_an_coords: .byte    0, 0
                  .byte    (14+OPPONENT_SPRITE_X)/7, OPPONENT_SPRITE_Y-OPPONENT_SPRITE_H+63
                  .byte    (28+OPPONENT_SPRITE_X)/7, OPPONENT_SPRITE_Y-OPPONENT_SPRITE_H+52
                  .byte    0, 0
                  .byte    0, 0
                  .byte    (35+OPPONENT_SPRITE_X)/7, OPPONENT_SPRITE_Y-OPPONENT_SPRITE_H+35
                  .byte    0, 0

NUM_ANIM_COORDS = (* - avatar_an_coords)/2
.assert NUM_ANIM_COORDS = NUM_SPRITES, error
