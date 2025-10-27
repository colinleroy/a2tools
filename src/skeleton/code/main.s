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

        .export   _main
        .export   _check_keyboard, _last_key
        .export   game_cancelled
        .export   _quit

        .import   _hgr_force_mono40
        .import   _mouse_setplaybox

        .import   _move_sprite

        .import   _freq
        .import   _mouse_wait_vbl

        .import   _platform_msleep
        .import   _build_hgr_tables
        .import   _init_caches

        .import   _draw_screen
        .import   _text_mono40, _home, _exit

        .import   ___randomize

        .include  "apple2.inc"
        .include  "constants.inc"
        .include  "freq.inc"
        .include  "sprite.inc"

.segment "CODE"

; How much do we have left?
; .res 1650

.proc _main
        jsr     _init_caches
        jsr     ___randomize
        jsr     _build_hgr_tables

        jmp     _real_main
.endproc

.proc pause
:       jsr     _check_keyboard
        bcc     :-
        lda     #$00
        sta     _last_key
        rts
.endproc

.proc _quit
        jmp     _exit
.endproc

.segment "LOWCODE"

.proc _check_keyboard
        lda     KBD
        bpl     no_kbd
        bit     KBDSTRB
        and     #$7F
        sta     _last_key
        sec
        rts
no_kbd:
        lda     #$00
        sta     _last_key
        clc
        rts
.endproc

.proc _real_main
        ; Bind mouse to table
        jsr     _mouse_setplaybox

game_loop:
        ; the WAI of the poor
        ; because I don't understand how WAI works
        ; and I want to keep things 6502-ok
        jsr     _mouse_wait_vbl

loop_start:
        ; Drop 1 frame out of 6 at 60Hz
        ; otherwise the game is harder
        lda     _freq

        .assert TV_NTSC = 0, error
        ; cmp     #TV_NTSC
        bne     frame_start
        dec     frame_counter
        bne     frame_start
        lda     #6
        sta     frame_counter
        jmp     game_loop

frame_start:
        ; First thing is drawing the screen so we don't flicker
        jsr     _draw_screen

draw_done:
        ; Move sprite
        jsr     _move_sprite

        ; Check for keyboard input
        jsr     _check_keyboard
        bcc     game_loop

        ; Keyboard hit, is it escape?
        cmp     #CH_ESC
        bne     game_loop
        jmp     _quit
.endproc

.bss
frame_counter:          .res 1
game_cancelled:         .res 1
_last_key:              .res 1
