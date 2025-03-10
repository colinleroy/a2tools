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

        .export     _check_blockers, _check_vents, _check_collisions
        .export     _draw_screen
        .export     _move_plane
        .export     _check_level_change
        .export     _check_fire_button

        .import     rubber_band_data
        .import     _check_plane_bounds, _check_rubber_band_bounds

        .import     _keyboard_update_ref_x
        .import     _keyboard_check_fire
        .import     _mouse_update_ref_x
        .import     _mouse_check_fire

        .import     _load_sprite_pointer
        .import     _setup_sprite_pointer
        .import     _draw_sprite

        .import     _fire_rubber_band
        .import     _rubber_band_travel

        .import     frame_counter, plane_sprite_num
        .import     _go_to_prev_level, _go_to_next_level, _go_to_level
        .import     keyboard_level_change

        .import     _inc_score
        .import     _unfire_sprite

        .import     _play_bubble

        .include    "level_data_ptr.inc"
        .include    "plane.gen.inc"
        .include    "sprite.inc"
        .include    "plane_coords.inc"
        .include    "constants.inc"
        .include    "levels_data/level_data_struct.inc"

.segment "LOWCODE"

; Return with carry set if in an obstacle
.proc _check_blockers
        ; Get current blockers data to data_ptr
        lda     LEVEL_DATA_START+LEVEL_DATA::BLOCKERS_DATA
        sta     data_ptr
        lda     LEVEL_DATA_START+LEVEL_DATA::BLOCKERS_DATA+1
        sta     data_ptr+1

        ; Get number of blockers in the level, to X
        ldy     #0
        lda     (data_ptr),y
        tax

        beq     no_blockers

do_check_blocker:
        iny
        jsr     _check_plane_bounds
        bcc     next_blocker

        ; We're in an obstacle
        lda     $C030
.ifdef UNKILLABLE
        clc                       ; NO BLOCKER HACK
.endif
        rts                       ; Carry already set

next_blocker:
        dex                       ; Check next obstacle?
        bne     do_check_blocker

no_blockers:
        clc
        rts
.endproc

; Return with Y increment to use
.proc _check_vents
.ifdef UNKILLABLE
        lda     #0                ; NO VENT HACK
        rts
.endif
        ; Get current vents data to data_ptr
        lda     LEVEL_DATA_START+LEVEL_DATA::VENTS_DATA
        sta     data_ptr
        lda     LEVEL_DATA_START+LEVEL_DATA::VENTS_DATA+1
        sta     data_ptr+1

        ; Get number of vents in the level, to X
        ldy     #0
        lda     (data_ptr),y
        tax

        beq     go_down

do_check_vent:
        iny
        jsr     _check_plane_bounds
        bcc     next_vent

        ; We're in a vent tunnel, load its Y delta
        iny
        lda     (data_ptr),y
        rts

next_vent:
        iny
        dex                       ; Check next vent?
        bne     do_check_vent

go_down:
        lda     #1              ; Go down normal
        rts
.endproc

.proc _draw_screen
        lda     plane_sprite_num
        sta     cur_sprite

        ; Always draw the plane
        jsr     _load_sprite_pointer
        jsr     _setup_sprite_pointer
        jsr     _draw_sprite

        lda     frame_counter     ; Draw only half sprites
        and     #01
        beq     :+
        dec     cur_sprite
:

draw_next_sprite:
        dec     cur_sprite
        lda     cur_sprite
        bmi     draw_done         ; All done!

        jsr     _load_sprite_pointer
        bne     dec_sprite_draw   ; Only draw dynamic sprites

        ldy     #SPRITE_DATA::ACTIVE
        lda     (cur_sprite_ptr),y
        beq     dec_sprite_draw   ; Only draw active sprites

        jsr     _setup_sprite_pointer
        jsr     _draw_sprite

dec_sprite_draw:
        dec     cur_sprite        ; Skip a sprite
        bpl     draw_next_sprite

draw_done:
        rts
.endproc

.proc _check_collisions
        ldx     plane_sprite_num
        stx     cur_sprite

check_next_sprite:
        dec     cur_sprite
        lda     cur_sprite
        bpl     :+                ; Are we done?
        clc
        rts

:       jsr     _load_sprite_pointer
        ldy     #SPRITE_DATA::ACTIVE
        lda     (cur_sprite_ptr),y
        beq     check_next_sprite

        ; Let's check whether a rubber band can destroy this sprite
        lda     rubber_band_data+SPRITE_DATA::ACTIVE
        beq     :+
        ldy     #SPRITE_DATA::DESTROYABLE
        lda     (cur_sprite_ptr),y
        beq     :+

        ; We have an in-flight rubber band and that sprite is destroyable
        ldy     #SPRITE_DATA::X_COORD
        jsr     _check_rubber_band_bounds
        bcs     destroy_sprite_with_rubber_band

:       ; Let's check the sprite's box
        .assert data_ptr = cur_sprite_ptr, error
        ldy     #SPRITE_DATA::X_COORD
        jsr     _check_plane_bounds
.ifdef UNKILLABLE
        bcc     :+
        clc                       ; NO COLLISION HACK
        sta     $C030
:
.endif
        bcc     check_next_sprite

        ; We're in the sprite box, is it active?
        ldy     #SPRITE_DATA::ACTIVE
        lda     (cur_sprite_ptr),y
        beq     check_next_sprite  ; No, we're good

        ; Is it deadly?
        ldy     #SPRITE_DATA::DEADLY
        lda     (cur_sprite_ptr),y
        beq     grab_sprite       ; No, grab it
        sec                       ; Yes, signal to die
        rts

destroy_sprite_with_rubber_band:
        lda     #0                ; Deactivate rubber band
        jsr     _unfire_sprite
        jsr     _play_bubble      ; Play sound
        lda     #DESTROY_BONUS
        jsr     _inc_score

        ; Unfire it
grab_sprite:
        lda     cur_sprite
        jsr     _unfire_sprite

        jmp     check_next_sprite
.endproc

.proc _move_plane
        jsr     _mouse_update_ref_x     ; Potentially patched out by main.s
        bcs     :+
        jsr     _keyboard_update_ref_x
:       sta     plane_x

        jsr     _check_vents            ; Returns with offset to add to plane_y
        clc
        adc     plane_y
        cmp     #plane_MAX_Y
        bcc     :+

        ; We're on the floor
        lda     #plane_MAX_Y

:       sta     plane_y
        rts
.endproc

.proc _check_level_change
        ; Check level change now
        ; First by cheat-code
        ldx     #$FF
        lda     keyboard_level_change
        stx     keyboard_level_change
        bmi     :+
        jmp     _go_to_level      ; End of function

:       ; Then by plane X coord
        lda     plane_x
        bne     :+
        jmp     _go_to_prev_level ; End of function

:       .assert (280-plane_WIDTH) .mod $2 = $0, error
        cmp     #(280-plane_WIDTH)
        bcs     :+
        rts
        ; We finished the level!
:       jmp     _go_to_next_level

.endproc

.proc _check_fire_button
        jsr     _mouse_check_fire ; Potentially NOP'd by main.s
        bcs     :+
        jsr     _keyboard_check_fire
        bcc     :++
:       jsr     _fire_rubber_band

:       jmp     _rubber_band_travel
.endproc

.bss

cur_sprite:     .res 1
