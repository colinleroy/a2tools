        .export   _main
        .export   frame_counter, time_counter

        .export   _go_to_prev_level, _go_to_next_level
        .export   _go_to_level

        .import   _exit
        .import   _init_hgr, _init_mouse, _deinit_mouse
        .import   _bzero
        .import   pushax

        .import   _build_hgr_tables
        .import   _mod7_table

        .import   cur_level
        .import   _draw_sprite
        .import   _load_sprite_pointer
        .import   _setup_sprite_pointer

        .import   _check_blockers, _check_collisions
        .import   _draw_screen, _draw_dashboard, _draw_level_end
        .import   _move_plane
        .import   _check_level_change
        .import   _check_fire_button

        .import   _load_level_data, _load_splash_screen, _load_lowcode
        .import   _animate_plane_crash

        .import   levels_logic, cur_level_logic

        .import   num_lives, num_rubber_bands, num_battery, cur_score
        .import   plane_sprite_num

        .import   vbl_ready, hz

        .import   _mouse_wait_vbl
        .import   _mouse_reset_ref_x
        .import   _mouse_calibrate_hz

        .import   _softswitch_wait_vbl
        .import   _keyboard_reset_ref_x
        .import   _keyboard_calibrate_hz

        .import   sprite_data, plane_data, rubber_band_data

        .import   _wait_for_input, _clear_hgr_screen
        .import   _platform_msleep

        .import   _play_crash
        .import   _load_and_show_high_scores
        .import   _init_text, _clrscr

        .importzp ptr2

        .include  "apple2.inc"
        .include  "plane.gen.inc"
        .include  "sprite.inc"
        .include  "level_data_ptr.inc"
        .include  "plane_coords.inc"
        .include  "constants.inc"
        .include  "levels_data/level_data_struct.inc"

.segment "LOWCODE"

.proc _go_to_prev_level
        rts ; UNSURE IF I WANT TO GO BACK IN LEVELS?
        ; We restore level data, in case we die later
        ; and come back to this level.
        lda     cur_level
        bne     :+
        rts
:       dec     cur_level
        jmp     load_level
.endproc

.proc _go_to_next_level
        inc     cur_level
        ; Print the inter-level screen
        jsr     _clear_hgr_screen
        jsr     _clrscr
        jsr     _init_text
        clc
        jsr     _draw_level_end
        ; We restore level data, in case we die later
        ; and come back to this level.
        lda     cur_level
        ; Fallthrough to _go_to_level
.endproc

.proc _go_to_level
        sta     cur_level
        jsr     load_level
        bcc     :+
        ; If we couldn't load another level, we win
        ; Refresh the end level screen with carry set
        jsr     _clear_hgr_screen
        jsr     _clrscr
        jsr     _init_text
        sec
        jsr     _end_game
:
        lda     #1
        jmp     _init_hgr
.endproc

.proc _lose_game
        jsr     _clear_hgr_screen
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
        jsr     _load_lowcode
        jmp     _real_main
.endproc

.segment "LOWCODE"

.proc _real_main
        jsr     _build_hgr_tables

        jsr     _init_mouse
        bcc     :+                ; Do we have a mouse?

        ; No. Patch functions to not require it
        lda     #<_softswitch_wait_vbl
        sta     wait_vbl_handler+1
        lda     #>_softswitch_wait_vbl
        sta     wait_vbl_handler+2

        lda     #<_keyboard_calibrate_hz
        sta     calibrate_hz_handler+1
        lda     #>_keyboard_calibrate_hz
        sta     calibrate_hz_handler+2

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

calibrate_hz_handler:
:       jsr     _mouse_calibrate_hz

.ifndef __APPLE2ENH__
        ; Give the Mousecard time to settle post-init
        lda     #$FF
        ldx     #0
        jsr     _platform_msleep
.endif

new_game:
        jsr     _load_splash_screen

        lda     #1
        jsr     _init_hgr

        jsr     _wait_for_input
        jsr     _clear_hgr_screen

        jsr     reset_game        ; Reset game loads current level (0)

game_loop:
        ; the WAI of the poor
        ; because I don't understand how WAI works
        ; and I want to keep things 6502-ok
wait_vbl_handler:
        jsr     _mouse_wait_vbl


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
        lda     hz                      ; Reset time counter frames to Hz
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
        dec     num_lives
        beq     game_over
        jmp     load_level
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
        lda     hz
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
        lda     #0
        sta     cur_level
        sta     num_rubber_bands
        sta     num_battery
        sta     cur_score
        sta     cur_score+1
        jmp     load_level
.endproc

_win:
        jsr     _deinit_mouse
        jmp     _exit

        .bss

frame_counter:   .res 1
time_counter:    .res 2
cur_sprite:      .res 1
