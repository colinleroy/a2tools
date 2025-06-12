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
; Two-players code - play with a friend

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

        .import     _cout, _strout, _gotoxy, _gotox, _gotoy, _home, _numout

        .import     _big_draw_sprite_n_1    ; Susan
        .import     _big_draw_sprite_n_2    ; Steve
        .import     _big_draw_sprite_n_3    ; Mx Raccoon
        .import     _big_draw_sprite_n_4, _big_draw_lose_n_4, _big_draw_win_n_4    ; Calvin
        .import     _big_draw_sprite_n_5, _big_draw_lose_n_5, _big_draw_win_n_5    ; Mafalda
        .import     _big_draw_sprite_n_6, _big_draw_lose_n_6, _big_draw_win_n_6    ; Luigi
        .import     _big_draw_sprite_n_7    ; Smudge
        .import     _big_draw_name_n
        .import     _update_opponent
        .import     _play_puck_hit

        .import     ostype, _platform_msleep

        .import     return0

        .import     __OPPONENT_START__

        .import     pusha, pushax, popax

        ; transport-specific calls
        .export     _conf_help_str

        .import     _configure_serial, _teardown_serial
        .import     _send_byte_serial, _get_byte_serial
        ; end of transport-specific calls

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

.segment "n"                                                        ; CHANGE A

.assert * = __OPPONENT_START__+OPPONENT::SPRITE, error ; Make sure the callback is where we think
        jmp     _big_draw_sprite_n_3                                    ; CHANGE A

.assert * = __OPPONENT_START__+OPPONENT::NAME, error ; Make sure the callback is where we think
        jmp     _big_draw_name_n                                        ; CHANGE A

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
        jsr     configure_network

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
        jsr     send_byte
        rts
.endproc

.proc send_miss_packet
:       lda     #IO_BARRIER         ; This needs sync, send a barrier
        DBG     $2023
        jsr     send_byte
        jsr     get_byte            ; And wait for its ack
        bcs     cancel
        cmp     #IO_BARRIER
        bne     :-
        DBG     $2024

        lda     #'M'                ; Send Miss
        jsr     send_byte
        DBG     $2025
        jsr     get_byte            ; Get ack
        DBG     $2026
cancel:
        rts
.endproc

.proc send_puck_packet
:       lda     #IO_BARRIER         ; This needs sync, send a barrier
        DBG     $2023
        jsr     send_byte
        jsr     get_byte            ; And wait for its ack
        bcs     cancel
        cmp     #IO_BARRIER
        bne     :-
        DBG     $2024

        lda     #$00             ; Reset hit indicator
        sta     player_puck_delta_change

        lda     #'H'             ; Tell we did hit
        DBG     $3022
        jsr     send_byte
        jsr     get_byte
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
        jsr     send_byte
        jsr     get_byte
        bcs     cancel
        ; Ignore the reply
        lda     puck_dy          ; Send reverted puck_dy
        NEG_A
        DBG     $3025
        jsr     send_byte
        jsr     get_byte
        bcs     cancel
        ; Ignore the reply

        lda     puck_x          ; Send mirrored puck_x
        jsr     mirror_puck_x
        DBG     $3026
        jsr     send_byte
        jsr     get_byte
        bcs     cancel
        ; Ignore the reply

        lda     puck_y          ; Send mirrored puck_y
        jsr     mirror_puck_y
        DBG     $3027
        jsr     send_byte
        jsr     get_byte
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
        jsr     send_byte
        DBG     $3023
        jsr     get_byte
        bcs     cancel
        DBG     $3024
        sta     puck_dx
        lda     #'?'              ; Get puck_dy
        jsr     send_byte
        jsr     get_byte
        bcs     cancel
        DBG     $3025
        sta     puck_dy

        lda     #'?'              ; Get puck_x
        jsr     send_byte
        jsr     get_byte
        bcs     cancel
        DBG     $3026
        sta     puck_x
        lda     #'?'              ; Get puck_y
        jsr     send_byte
        jsr     get_byte
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

        beq     get_and_send_data   ; Otherwise we have nothing special to say

must_send_miss:
        jsr     send_miss_packet
        bcs     cancel
        jmp     puck_crashed        ; And get out

must_send_puck_params:
        jsr     send_puck_packet    ; Send puck params
        jmp     clc_out             ; And get out

get_and_send_data:
        jsr     try_to_get_byte     ; Do we have a message,
        bcs     send_coords         ; No, send our coordinates
        DBG     $2023
        cmp     #IO_BARRIER         ; Yes, is it a barrier?
        bne     :+
        jsr     send_byte           ; Yes, ack it
        DBG     $2024
        jmp     do_read             ; And go read the message

:       jsr     unpack_pusher_coords; We only got coords,
        stx     their_pusher_x      ; Update them
        sty     their_pusher_y

send_coords:
        jsr     send_pusher_coords  ; And send our own coordinates
        jmp     clc_out

do_read:
        jsr     get_byte            ; Get the message
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
        jsr     send_byte           ; Send ack
puck_crashed:
        plp
        sec                         ; Caller expects carry set when puck crashed

out:    lda     #$00
        DBG     $2023
        DBG     $2024
        DBG     $2025
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

.proc configure_network
        jsr     _home
        jsr     _hgr_mixon
        jsr     _hgr_unset_mono40

        ; Hide the connection type chooser as long as there
        ; is only one
        lda     #NUM_CONNECTION_TYPES
        cmp     #1
        beq     configuration_done

choose_connection:
        jsr     _home
        lda     #0
        jsr     pusha
        lda     #20
        jsr     _gotoxy

        lda     #<conn_type_str
        ldx     #>conn_type_str
        jsr     _strout

        lda     connection_type
        asl
        tay
        lda     connection_types,y
        iny
        ldx     connection_types,y
        jsr     _strout

update:
        ldx     tmp_param
        jsr     _read_key
        cmp     #$08
        beq     dec_param
        cmp     #$15
        beq     inc_param
        cmp     #$0D              ; Enter
        beq     configuration_done
        cmp     #CH_ESC
        beq     cancel
        bne     choose_connection
dec_param:
        lda     connection_type
        beq     choose_connection
        dec     connection_type
        jmp     choose_connection
inc_param:
        ldx     connection_type
        inx
        cpx     #NUM_CONNECTION_TYPES
        beq     choose_connection
        inc     connection_type
        jmp     choose_connection

cancel:
        inc     game_cancelled
        rts

configuration_done:
        jsr     patch_transport_endpoints

        jsr     configure_transport
        bcs     finish_game

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
.endproc

.proc finish_game
        jsr     teardown_transport

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

        jsr     send_byte
        jsr     try_to_get_byte
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

        jsr     send_byte
:       jsr     get_byte
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

; Sends A over transport, receives A over transport
.proc exchange_char
        php
        sei
        jsr     send_byte           ; Send char
        jsr     get_byte            ; Get remote char (or escape)
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

; Try 20 times to get a char.
; (20 times to make sure the other side has time to send)
; Destroys X, does not touch Y.
; Returns with carry set if no byte is available or the
; player pressed escape, or with the read byte in A and
; carry clear.
.proc try_to_get_byte
        lda     #20
        sta     net_timer

try:    jsr     maybe_get_byte
        bcc     out

        dec     net_timer
        bne     try

        jsr     check_escape
        bcs     out_abort
        sec

out:    rts
out_abort:
        inc     game_cancelled
        rts
.endproc

; Do get a byte from the transport layer, waiting
; infinitely if necessary; but exits with carry
; set if the player pressed escape.
.proc get_byte
        lda     game_cancelled
        bne     out
:       jsr     maybe_get_byte
        bcc     out
        jsr     check_escape
        bcc     :-
out:    rts
.endproc

.proc patch_transport_endpoints
        lda     connection_type
        asl
        tay
        lda     conn_configure_cbs,y
        sta     configure_transport+1
        lda     conn_teardown_cbs,y
        sta     teardown_transport+1
        lda     conn_send_cbs,y
        sta     send_byte+1
        lda     conn_get_cbs,y
        sta     maybe_get_byte+1

        iny
        ldx     conn_configure_cbs,y
        lda     conn_configure_cbs,y
        sta     configure_transport+2
        lda     conn_teardown_cbs,y
        sta     teardown_transport+2
        lda     conn_send_cbs,y
        sta     send_byte+2
        lda     conn_get_cbs,y
        sta     maybe_get_byte+2

        rts
.endproc

; ========= Transport-specific functions, patched with callbacks ======

; maybe_get_byte must load the read byte in A or return with carry set
; if no byte is available on the transport layer.
.proc maybe_get_byte
        jmp     $FFFF
.endproc

; send_byte must send the byte in A.
.proc send_byte
        jmp     $FFFF
.endproc

; Setup the transport layer (serial slot)
.proc configure_transport
        jmp     $FFFF
.endproc

; Cleanup the transport layer
.proc teardown_transport
        jmp     $FFFF
.endproc

; ========= End of transport-specific functions ======

net_timer:        .word   0
connected:        .byte   0
player:           .byte   0

tmp_param:        .byte   0
my_avatar_num:    .byte   0
their_avatar_num: .byte   0
iigs_spd:         .byte   0
barrier_tries:    .byte   0

ident_str:        .asciiz "SHFL1"

wait_str:         .asciiz "WAITING FOR PLAYER"

_conf_help_str:   .asciiz "ARROW KEYS TO CHANGE, ENTER TO VALIDATE"
ready_str:        .asciiz "                 READY!                "

; Transport-related options
conn_type_str:    .asciiz "CONNECTION TYPE: "
connection_type:  .byte   0

; Add different transport layers by adding
; - one connection type string
; - that string in the connection_types array
; - your callbacks (configure, teardown, send, get)
;   in the relevant arrays.
serial_conn_str:  .asciiz "SERIAL"
;dummy_conn_str:   .asciiz "DUMMY"

connection_types: .addr   serial_conn_str
;                  .addr   dummy_conn_str
NUM_CONNECTION_TYPES = (* - connection_types)/2
conn_configure_cbs:
                  .addr   _configure_serial
;                  .addr   $0000
conn_teardown_cbs:.addr   _teardown_serial
;                  .addr   $0000
conn_send_cbs:    .addr   _send_byte_serial
;                  .addr   $0000
conn_get_cbs:     .addr   _get_byte_serial
;                  .addr   $0000

; Avatar options
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

avatar_sprites:   .addr   _big_draw_sprite_n_3
                  .addr   _big_draw_sprite_n_4
                  .addr   _big_draw_sprite_n_5
                  .addr   _big_draw_sprite_n_1
                  .addr   _big_draw_sprite_n_2
                  .addr   _big_draw_sprite_n_6
                  .addr   _big_draw_sprite_n_7

NUM_SPRITES = (* - avatar_sprites)/2
.assert NUM_AVATARS = NUM_SPRITES, error

avatar_animations:.addr    return0, return0
                  .addr    _big_draw_win_n_4, _big_draw_lose_n_4
                  .addr    _big_draw_win_n_5, _big_draw_lose_n_5
                  .addr    return0, return0
                  .addr    return0, return0
                  .addr    _big_draw_win_n_6, _big_draw_lose_n_6
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
