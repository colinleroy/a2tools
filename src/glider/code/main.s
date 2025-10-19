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
        .export   frame_counter, time_counter

        .export   _go_to_level
        .export   _initial_plane_x, _initial_plane_y

        .import   _init_mouse

        .import   cur_level
        .import   _draw_sprite
        .import   _load_sprite_pointer
        .import   _setup_sprite_pointer

        .import   _check_blockers, _check_collisions
        .import   _draw_screen, _draw_level_end
        .import   _draw_dashboard, _draw_dashboard_background
        .import   _move_plane
        .import   _check_level_change
        .import   _check_fire_button

        .import   _load_level_data, _load_splash_screen, _load_lowcode
        .import   _animate_plane_crash

        .import   cur_level_logic

        .import   num_lives, num_rubber_bands, num_battery, cur_score
        .import   plane_sprite_num, level_done

        .import   _freq

        .import   _mouse_wait_vbl
        .import   _mouse_reset_ref_x

        .import   _softswitch_wait_vbl
        .import   _keyboard_reset_ref_x

        .import   plane_data, rubber_band_data

        .import   _wait_for_input
        .import   _platform_msleep
        .import   _allow_lowercase
        .import   _build_hgr_tables

        .import   _play_crash
        .import   _text_mono40, _hgr_force_mono40, _load_hgr_mono_file, _clrscr

        .include  "apple2.inc"
        .include  "plane.gen.inc"
        .include  "sprite.inc"
        .include  "level_data_ptr.inc"
        .include  "plane_coords.inc"
        .include  "constants.inc"
        .include  "levels_data/level_data_struct.inc"

.segment "LOWCODE"

; A contains the level to go to.
; X contains the plane X at start of level
; Y contains the plane Y at start of level
; X or Y can be $FF for no change
.proc _go_to_level
        stx     _initial_plane_x
        sty     _initial_plane_y

        ldx     cur_level         ; Get the previous cur_level before overwriting it
        stx     prev_level
        ldy     level_done,x      ; Check whether we already did that level
        sty     no_inter_screen

        sta     cur_level         ; Store new current level

        tax
        lda     level_done,x      ; Are we going back to an already done level?
        ora     no_inter_screen   ; No inter-level screen either in this case
        sta     no_inter_screen

        jsr     _clrscr
        jsr     _text_mono40
        clc
        lda     no_inter_screen   ; Should we show the inter level screen?
        bne     :+

        ldx     prev_level
        lda     #1
        sta     level_done,x      ; Mark the level we come from as done
        jsr     _draw_level_end

:       jsr     load_level
        bcc     :+
        ; If we couldn't load another level, we win
        ; Refresh the end level screen with carry set
        sec
        jsr     _end_game
:
        jmp     _hgr_force_mono40
.endproc

.proc _lose_game
        jsr     _clrscr
        jsr     _text_mono40
        clc
        ; Fallthrough to _end_game
.endproc

.proc _end_game
        jsr     _draw_level_end
        ; And set the level to -1 to signal the game loop
        lda     #$FF
        sta     cur_level
        rts
.endproc

.proc load_level
        lda     cur_level
        jsr     _load_level_data
        bcc     :+
        rts
        ; Draw plane once to backup background
:       jsr     setup_level_data
x_coord_reset_handler:
        jsr     _mouse_reset_ref_x
        jsr     _keyboard_reset_ref_x
        clc
        rts
.endproc

.code

.proc _main
        lda     #1
        jsr     _load_hgr_mono_file
        jsr     _load_lowcode
        jmp     _real_main
.endproc

.segment "LOWCODE"

.proc _real_main
        jsr     _build_hgr_tables

        lda     #1
        jsr     _allow_lowercase  ; For Bulgarian i18n

        jsr     _init_mouse
        bcc     :+                ; Do we have a mouse?

        ; No. Patch functions to not require it
        lda     #<_softswitch_wait_vbl
        sta     wait_vbl_handler+1
        lda     #>_softswitch_wait_vbl
        sta     wait_vbl_handler+2

        lda     #<_keyboard_reset_ref_x
        sta     load_level::x_coord_reset_handler+1
        lda     #>_keyboard_reset_ref_x
        sta     load_level::x_coord_reset_handler+2

        ; Deactivate mouse (X and fire) handlers
        lda     #$18              ; CLC
        sta     _move_plane
        lda     #$EA              ; NOP
        sta     _move_plane+1
        sta     _move_plane+2

        lda     #$18              ; CLC
        sta     _check_fire_button
        lda     #$EA              ; NOP
        sta     _check_fire_button+1
        sta     _check_fire_button+2

:

        ; Give the Mousecard time to settle post-init
        lda     #$FF
        ldx     #0
        jsr     _platform_msleep

new_game:
        jsr     _load_splash_screen

        jsr     _hgr_force_mono40

        jsr     _wait_for_input
        cmp     #($1B|$80)        ; Escape?
        bne     :+
        jsr     _text_mono40      ; Switch back to text mode
        rts                       ; Final rts

:       jsr     _clrscr
        jsr     _text_mono40

        jsr     reset_game        ; Reset game loads current level (0)

        jsr     _hgr_force_mono40


game_loop:
        ; the WAI of the poor
        ; because I don't understand how WAI works
        ; and I want to keep things 6502-ok
wait_vbl_handler:
        jsr     _mouse_wait_vbl

loop_start:
; DRAW SPRITES FIRST
        jsr     _draw_screen

; DRAW DASHBOARD
        jsr     _draw_dashboard

; GENERAL GAME LOGIC
game_logic:
        ; Performance test here. Decomment for just the draw loop
        ; inc frame_counter
        ; jmp game_loop

        ; Check if a second elapsed
        dec     time_counter+1          ; Decrement time counter frames
        bne     :+
        lda     time_counter            ; Decrement time counter seconds if not 0
        beq     :+
        dec     time_counter
        ldx     _freq                   ; Reset time counter frames to Hz
        lda     hz_vals,x
        sta     time_counter+1

:       ; Check coordinates and update them depending on vents
        jsr     _move_plane

        ; Check obstacles
        jsr     _check_blockers
        bcc     :+
        ; We got in an obstacle
        jsr     die

:       ; Check if we're done with the level
        jsr     _check_level_change

        ; Check if we finished the game, which we know because
        ; the level will have been set to $FF
        lda     cur_level
        bmi     new_game

        ; Hook through the level's logic
        jsr     LEVEL_DATA_START+LEVEL_DATA::LOGIC_CB

        ; Check if we should fire a rubber band
        jsr     _check_fire_button

        inc     frame_counter

;
; COLLISION CHECKS, after updating positions and before
; redrawing
;
        jsr     _check_collisions
        bcc     :+
        jsr     die

        ; Next round!
:       jmp     game_loop
.endproc

.proc die
        jsr     _animate_plane_crash
        jsr     _play_crash

        lda     #PLANE_ORIG_X
        sta     _initial_plane_x
        lda     #PLANE_ORIG_Y
        sta     _initial_plane_y

        dec     num_lives
        beq     game_over
        jsr     _clrscr
        jsr     _text_mono40
        jsr     load_level
        lda     #1
        jmp     _hgr_force_mono40
game_over:
        jmp     _lose_game
.endproc

.proc setup_level_data
        ; Logic
        lda     LEVEL_DATA_START+LEVEL_DATA::LOGIC_CB
        sta     cur_level_logic
        lda     LEVEL_DATA_START+LEVEL_DATA::LOGIC_CB+1
        sta     cur_level_logic+1

        ; Allocated time to time counter seconds
        lda     #ALLOCATED_TIME
        sta     time_counter

        ; Reset frame counter
        lda     #$00
        sta     frame_counter

        ; Set time counter frames
        ldx     _freq
        lda     hz_vals,x
        sta     time_counter+1

        ; Sprites
        lda     LEVEL_DATA_START+LEVEL_DATA::SPRITES_DATA
        sta     level_data
        lda     LEVEL_DATA_START+LEVEL_DATA::SPRITES_DATA+1
        sta     level_data+1

        ldy     #0
        lda     (level_data),y

        ; Remember the plane sprite number
        sec
        sbc     #1
        sta     plane_sprite_num

        ; Move pointer to actual data now we have the number
        ; of sprites
        inc     level_data
        bne     :+
        inc     level_data+1

:       ; Mark plane and rubber band backgrounds clean
        lda     #0
        sta     plane_data+SPRITE_DATA::NEED_CLEAR
        sta     rubber_band_data+SPRITE_DATA::NEED_CLEAR

        ; Deactivate interrupts for first draw
        php
        sei

        ; Clear the dashboard background
        jsr     _draw_dashboard_background

        ; Draw each sprite once
        lda     plane_sprite_num
        sta     cur_sprite

:       jsr     _load_sprite_pointer
        beq     :+                      ; Only draw static sprites
        jsr     _setup_sprite_pointer
        jsr     _draw_sprite

:       dec     cur_sprite
        lda     cur_sprite
        bpl     :--

        plp
        rts
.endproc

.proc reset_game
        lda     #NUM_LIVES
        sta     num_lives
        lda     #PLANE_ORIG_X
        sta     _initial_plane_x
        lda     #PLANE_ORIG_Y
        sta     _initial_plane_y
        lda     #0
        sta     cur_level
        sta     num_rubber_bands
        sta     num_battery
        sta     cur_score
        sta     cur_score+1
        ldy     #MAX_LEVELS
:       sta     level_done,y
        dey
        bpl     :-
        ; Go to level 0
        jmp     load_level
.endproc

        .data

hz_vals:
        .byte   60                ; TV_NTSC
        .byte   50                ; TV_PAL

        .bss

frame_counter:   .res 1
time_counter:    .res 2
cur_sprite:      .res 1
_initial_plane_x:.res 1
_initial_plane_y:.res 1
no_inter_screen: .res 1
prev_level:      .res 1
