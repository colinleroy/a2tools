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

        .export     _softswitch_wait_vbl
        .export     _keyboard_reset_ref_x
        .export     _keyboard_update_ref_x
        .export     _keyboard_check_fire
        .export     _read_key
        .export     keyboard_level_change

        .import     prev_x, ref_x, mouse_x
        .import     _check_battery_boost
        .import     cur_level
        .import     _initial_plane_x, _initial_plane_y
        .include    "apple2.inc"
        .include    "sprite.inc"
        .include    "plane_coords.inc"
        .include    "constants.inc"

.segment "CODE"

.proc _read_key
        lda     KBD
        bpl     _read_key
        bit     KBDSTRB
        and     #$7F
        rts
.endproc

.segment "LOWCODE"

; $C019  Bit 7 of this location will be a 0 when the
; video beam is in the vertical retrace region.

.proc _softswitch_wait_vbl
        bit     $C019               ; Softswitch VBL
        bpl     _softswitch_wait_vbl; Wait for bit 7 on (VBL ended)
:       bit     $C019               ; Softswitch VBL
        bmi     :-                  ; Wait for bit 7 off (VBL started)
        rts
.endproc

.proc _keyboard_reset_ref_x
        bit     KBDSTRB
        lda     #$00
        sta     kbd_should_fire

        lda     _initial_plane_x
        cmp     #$FF
        beq     :+
        sta     ref_x
        sta     mouse_x
        sta     plane_x
        sta     prev_x

:       lda     _initial_plane_y
        cmp     #$FF
        beq     :+
        sta     plane_y
:       rts
.endproc

.proc _keyboard_update_ref_x
        lda     #$FF
        sta     keyboard_level_change

        lda     ref_x
        ldy     KBDSTRB
        cpy     #($08|$80)        ; CURS_LEFT
        beq     kbd_neg
        cpy     #($15|$80)        ; CURS_RIGHT
        beq     kbd_pos
        cpy     #($20|$80)       ; SPACE
        beq     kbd_fire
        cpy     #('p'|$80)       ; p
        beq     kbd_pause
        cpy     #('P'|$80)       ; P
        beq     kbd_pause
        cpy     #($1B|$80)       ; Escape
        beq     kbd_escape
.ifdef UNKILLABLE
        cpy     #($0B|$80)       ; CURS_UP
        beq     kbd_up
        cpy     #($0A|$80)       ; CURS_DOWN
        beq     kbd_down
.endif
        bit     BUTN0
        bpl     kbd_out
        ; Open-Apple is down. Clear high bit, check the key
        tya
        and     #$7F
        cmp     #'g'              ; G to change level
        bne     kbd_out

        ; It is, now get key and go to level
        bit     KBDSTRB
        jsr     _read_key
        sec
        sbc     #'a'
        sta     keyboard_level_change
        jmp     kbd_out

.ifdef UNKILLABLE
kbd_up:
        dec     plane_y
        jmp     kbd_out
kbd_down:
        inc     plane_y
        jmp     kbd_out
.endif
kbd_pos:
        clc
        adc     #PLANE_VELOCITY
        bcs     kbd_out
        sta     ref_x

        jsr     _check_battery_boost
        jmp     kbd_out

kbd_neg:
        sec
        sbc     #PLANE_VELOCITY
        bcc     kbd_out
        sta     ref_x
        jmp     kbd_out

kbd_fire:
        lda     #1
        sta     kbd_should_fire
        jmp     kbd_out

kbd_pause:
:       lda     KBD               ; Just wait for another keypress
        bpl     :-
        bit     KBDSTRB
        jmp     kbd_out

kbd_escape:
        lda     #$FF
        sta     cur_level

kbd_out:
        lda     ref_x
        rts
.endproc

.proc _keyboard_check_fire
        lda     kbd_should_fire
        beq     :+
        lda     #$00
        sta     kbd_should_fire
        sec
        rts
:       clc
        rts
.endproc

        .data

keyboard_level_change: .byte $FF

        .bss

kbd_should_fire:       .res 1
