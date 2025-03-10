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

        .export     _check_plane_bounds, _check_rubber_band_bounds
        .export     _unfire_sprite
        .export     _fire_rubber_band, _fire_sprite
        .export     _rubber_band_travel, _balloon_travel
        .export     _knife_travel, _toast_travel
        .export     _socket_toggle
        .export     _grab_rubber_bands, _grab_battery, _grab_sheet, _inc_score
        .export     _clock_inc_score
        .export     _check_battery_boost
        .import     ref_x

        .import     rubber_band_data
        .import     frame_counter
        .import     _load_sprite_pointer, _setup_sprite_pointer, _draw_sprite
        .import     num_rubber_bands, num_battery, num_lives, cur_score
        .import     _play_bubble, _play_croutch, _play_ding

        .importzp   tmp2, tmp3

        .include    "apple2.inc"
        .include    "balloon.gen.inc"
        .include    "knife.gen.inc"
        .include    "plane.gen.inc"
        .include    "rubber_band.gen.inc"
        .include    "sprite.inc"
        .include    "plane_coords.inc"
        .include    "level_data_ptr.inc"
        .include    "constants.inc"

.segment "LOWCODE"

; Return with carry set if mouse coords in box
; (data_ptr),y to y+3 contains box coords (start X, width, start Y, height)
; Always return with Y at end of coords so caller knows where Y is at.
; Trashes A, updates Y, does not touch X
.proc _check_plane_bounds
        ; plane_x,plane_y is the top-left corner of the plane
        ; compute bottom-right corner
        clc
        lda     plane_x
        sta     sx+1
        adc     #plane_WIDTH
        sta     ex+1
        lda     plane_y
        sta     sy+1
        adc     #plane_HEIGHT
        sta     ey+1

do_check:
        ; Check right of plane against first blocker X coords
        lda     (data_ptr),y
        iny                       ; Inc Y now so we know how much to skip
ex:
        cmp     #$FF              ; lower X bound
        bcs     out_skip_y        ; if lb > x, we're out of box

        ; Check left of plane against right of box
        adc     (data_ptr),y      ; higher X bound (lower+width)
sx:
        cmp     #$FF              ; Patched with X coordinate
        bcc     out_skip_y        ; if hb < x, we're out of box

        ; Check bottom of plane against first blocker Y coords
        iny
        lda     (data_ptr),y      ; lower Y bound
        iny                       ; Inc Y now so we have nothing to skip
ey:
        cmp     #$FF
        bcs     out               ; if lb > y, we're out of box

        ; Check top of plane against lower bound of blocker
        adc     (data_ptr),y      ; higher Y bound (lower+height)
sy:
        cmp     #$FF
        bcc     out               ; if hb < y, we're out of box

        rts                       ; We're in the box (return, carry already set)

out_skip_y:
        iny
        iny
out:
        clc
        rts
.endproc

.proc _check_rubber_band_bounds
        clc
        lda     rubber_band_data+SPRITE_DATA::X_COORD
        sta     _check_plane_bounds::sx+1
        adc     #rubber_band_WIDTH
        sta     _check_plane_bounds::ex+1
        lda     rubber_band_data+SPRITE_DATA::Y_COORD
        sta     _check_plane_bounds::sy+1
        adc     #rubber_band_HEIGHT
        sta     _check_plane_bounds::ey+1
        jmp     _check_plane_bounds::do_check
.endproc

_deactivate_rubber_band:
        lda     #0

.proc _deactivate_sprite
        jsr     _load_sprite_pointer
        jsr     _setup_sprite_pointer
        ldy     #SPRITE_DATA::ACTIVE
        lda     #0
        sta     (cur_sprite_ptr),y
        ldy     #SPRITE_DATA::DEACTIVATE_FUNC+1
        lda     (cur_sprite_ptr),y
        beq     deac_cb_done
        sta     deac_cb+2
        dey
        lda     (cur_sprite_ptr),y
        sta     deac_cb+1
        ldy     #SPRITE_DATA::DEACTIVATE_DATA
        lda     (cur_sprite_ptr),y
deac_cb:
        jsr     $FFFF

deac_cb_done:
        jmp     _draw_sprite
.endproc

.proc _fire_rubber_band
        lda     rubber_band_data+SPRITE_DATA::ACTIVE
        bne     no_fire

        lda     num_rubber_bands
        beq     no_fire
        dec     num_rubber_bands

        lda     plane_x
        cmp     #(250-plane_WIDTH-1)
        bcs     no_fire

        adc     #(plane_WIDTH+1)
        sta     rubber_band_data+SPRITE_DATA::X_COORD
        sta     rubber_band_data+SPRITE_DATA::PREV_X_COORD

        lda     plane_y
        clc
        adc     #((plane_HEIGHT-rubber_band_HEIGHT)/2)
        sta     rubber_band_data+SPRITE_DATA::Y_COORD
        sta     rubber_band_data+SPRITE_DATA::PREV_Y_COORD

        lda     #1
        sta     rubber_band_data+SPRITE_DATA::ACTIVE

no_fire:
        rts
.endproc

.proc _rubber_band_travel
        lda     rubber_band_data+SPRITE_DATA::ACTIVE
        beq     no_travel

        lda     rubber_band_data+SPRITE_DATA::X_COORD
        cmp     #(250-plane_WIDTH-1)
        bcs     _deactivate_rubber_band

        adc     #3
        sta     rubber_band_data+SPRITE_DATA::X_COORD
no_travel:
        rts
.endproc

; X: frame number to trigger sprite
; A: sprite number
.proc _fire_sprite
        cpx     frame_counter
        bne     :+

        jsr     _load_sprite_pointer
        ldy     #SPRITE_DATA::ACTIVE
        lda     (cur_sprite_ptr),y
        bne     :+

        lda     #1
        sta     (cur_sprite_ptr),y

        ; Store its original coords
        ldy     #SPRITE_DATA::X_COORD
        lda     (cur_sprite_ptr),y
        ldy     #SPRITE_DATA::STATE_BACKUP
        sta     (cur_sprite_ptr),y

        ldy     #SPRITE_DATA::Y_COORD
        lda     (cur_sprite_ptr),y
        ldy     #SPRITE_DATA::STATE_BACKUP+1
        sta     (cur_sprite_ptr),y

:       rts
.endproc

; A: sprite number
.proc _unfire_sprite
        jsr     _deactivate_sprite

        ; Restore its original coords if saved
        ldy     #SPRITE_DATA::STATE_BACKUP
        lda     (cur_sprite_ptr),y
        tax
        iny
        ora     (cur_sprite_ptr),y
        beq     :+

        txa
        ldy     #SPRITE_DATA::X_COORD
        sta     (cur_sprite_ptr),y

        ldy     #SPRITE_DATA::STATE_BACKUP+1
        lda     (cur_sprite_ptr),y
        ldy     #SPRITE_DATA::Y_COORD
        sta     (cur_sprite_ptr),y
:       rts
.endproc

; A: sprite number
; X: whether the socket should be on
.proc _socket_toggle
        stx     tmp2
        sta     tmp3
        jsr     _load_sprite_pointer
        ldy     #SPRITE_DATA::ACTIVE

        ldx     tmp2
        beq     :+
        ; Activate the socket every three frames
        lda     frame_counter
        and     #$03
        beq     :+
        sta     $C030
        sta     (cur_sprite_ptr),y
        rts

:       lda     (cur_sprite_ptr),y
        beq     :+
        lda     tmp3
        jsr     _unfire_sprite
:       rts
.endproc

.proc _balloon_travel
        sta     tmp3
        jsr     _load_sprite_pointer
        ldy     #SPRITE_DATA::ACTIVE
        lda     (cur_sprite_ptr),y
        beq     :+
        ; If so, up it
        ldy     #SPRITE_DATA::Y_COORD
        lda     (cur_sprite_ptr),y
        sec
        sbc     #1
        sta     (cur_sprite_ptr),y
        bne     :+
        ; If Y = 0, deactivate it
        lda     tmp3
        jmp     _unfire_sprite
:       rts
.endproc

.proc _knife_travel
        sta     tmp3
        jsr     _load_sprite_pointer
        ldy     #SPRITE_DATA::ACTIVE
        lda     (cur_sprite_ptr),y
        bne     :+
        rts                       ; It's not active

:       ; If active, down it
        ldy     #SPRITE_DATA::Y_COORD
        lda     (cur_sprite_ptr),y
        clc
        adc     #1
        sta     (cur_sprite_ptr),y
        cmp     #190-knife_HEIGHT ; Is Y still over floor?
        bcc     knife_left
knife_out:
        ; No
        lda     tmp3
        jmp     _unfire_sprite

knife_left:
        ; Now left it
        ldy     #SPRITE_DATA::X_COORD
        lda     (cur_sprite_ptr),y
        sec
        sbc     #1
        sta     (cur_sprite_ptr),y
        beq     knife_out
        rts
.endproc

; Approximation of a parabolic trajectory
toast_y_offset:
        .byte $F7, $F8, $F8, $F9, $FA, $FB, $FB, $FC, $FC, $FD, $FD, $FE, $FE, $FE, $FF, $FF, $FF, $FF, $FF, $FF
        .byte $00, $00, $00, $00, $00, $00, $00, $00, $00
        .byte $01, $01, $01, $01, $01, $01, $02, $02, $02, $03, $03, $04, $04, $05, $05, $06, $07, $08, $08, $09
NUM_TOAST_OFFSETS = * - toast_y_offset

toast_x_offset:
        .byte $00, $00, $00, $00, $00, $00, $FF, $00, $00, $00, $00, $00, $00, $FF, $00, $00, $00, $00, $00, $00
        .byte $FF, $00, $00, $00, $00, $00, $00, $FF, $00
        .byte $00, $00, $00, $00, $00, $FF, $00, $00, $00, $00, $00, $00, $FF, $00, $00, $00, $00, $00, $00, $07
.assert * - toast_x_offset = NUM_TOAST_OFFSETS, error ; Both arrays should be the same size

.proc _toast_travel
        sta     tmp3
        jsr     _load_sprite_pointer
        ldy     #SPRITE_DATA::ACTIVE
        lda     (cur_sprite_ptr),y
        bne     :+
        rts

:       ; Figure out the travel step
        ldy     #SPRITE_DATA::DEACTIVATE_DATA
        lda     (cur_sprite_ptr),y
        bne     :+
        ; Start toast!
        jsr     _play_croutch
        ; Restore registers
        ldy     #SPRITE_DATA::DEACTIVATE_DATA
        lda     #$00
        beq     toast_move_step

:       cmp     #NUM_TOAST_OFFSETS-1
        beq     end_toast_move

toast_move_step:
        clc
        adc     #1
        sta     (cur_sprite_ptr),y
        tax
        dex                       ; Start at 0 in the array

        ldy     #SPRITE_DATA::Y_COORD
        lda     (cur_sprite_ptr),y
        clc
        adc     toast_y_offset,x
        sta     (cur_sprite_ptr),y

        ; Now twiggle X for rotation
        ldy     #SPRITE_DATA::X_COORD
        lda     (cur_sprite_ptr),y
        clc
        adc     toast_x_offset,x
        sta     (cur_sprite_ptr),y
        rts

end_toast_move:
        ; Reset the step indicator for next time,
        ldy     #SPRITE_DATA::DEACTIVATE_DATA
        lda     #0
        sta     (cur_sprite_ptr),y
        ; and deactivate the toast
        lda     tmp3
        jmp     _unfire_sprite
.endproc

.proc _grab_rubber_bands
        clc
        adc     num_rubber_bands
        bcs     :+
        sta     num_rubber_bands
        jsr     _play_croutch
        rts
:       lda     #$FF
        sta     num_rubber_bands
        rts
.endproc

.proc _clock_inc_score
        jsr     _play_ding
        lda     #5
        ; Fallthrough through _inc_score
.endproc

.proc _inc_score
        clc
        adc     cur_score
        sta     cur_score
        bcc     :+
        inc     cur_score+1
:       rts
.endproc

.proc _grab_battery
        clc
        adc     num_battery
        bcs     :+
        sta     num_battery
        jsr     _play_croutch
        rts
:       lda     #$FF
        sta     num_battery
        rts
.endproc

.proc _grab_sheet
        clc
        adc     num_lives
        bcs     :+
        sta     num_lives
        jsr     _play_bubble
        rts
:       lda     #$FF
        sta     num_lives
        rts
.endproc

.proc _check_battery_boost
        ; Do we have battery?
        ldy     num_battery
        beq     :+
        ; Does the player want a boost (Open-Apple key)?
        bit     BUTN0
        bpl     :+

        adc     #PLANE_VELOCITY
        bcs     :+            ; Don't overflow X
        sta     ref_x
        sta     $C030
        dec     num_battery   ; Decrement battery
:       rts
.endproc
