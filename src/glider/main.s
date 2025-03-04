        .export   _main
        .export   _hgr_low, _hgr_hi
        .export   frame_counter, time_counter

        .export   _go_to_prev_level, _go_to_next_level, _go_to_level

        .import   _exit
        .import   _init_hgr, _init_mouse, _deinit_mouse
        .import   _init_hgr_base_addrs, _hgr_baseaddr
        .import   _bzero
        .import   pushax
        .import   _mod7_table

        .import   cur_level, num_levels
        .import   _draw_sprite
        .import   _load_sprite_pointer
        .import   _setup_sprite_pointer

        .import   _check_blockers, _check_collisions
        .import   _draw_screen, _draw_dashboard, _draw_level_end
        .import   _move_plane
        .import   _check_level_change
        .import   _check_fire_button

        .import   _load_bg
        .import   _animate_plane_crash

        .import   level_backup
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

        .import   _clear_hgr_after_input

        .import   _play_crash

        .importzp ptr2

        .include  "apple2.inc"
        .include  "plane.gen.inc"
        .include  "sprite.inc"
        .include  "level_data_ptr.inc"
        .include  "plane_coords.inc"
        .include  "constants.inc"

.segment "LOWCODE"

.proc _go_to_prev_level
        rts ; UNSURE IF I WANT TO GO BACK IN LEVELS?
        ; We restore level data, in case we die later
        ; and come back to this level.
        lda     cur_level
        bne     :+
        rts
:       jsr     restore_level_data
        dec     cur_level
        jmp     load_level
.endproc

.proc _go_to_next_level
        inc     cur_level
        ; Print the time bonus
        jsr     _draw_level_end
        ; We restore level data, in case we die later
        ; and come back to this level.
        lda     cur_level
        ; Fallthrough to _go_to_level
.endproc

.proc _go_to_level
        sta     cur_level
        cmp     num_levels
        bcc     :+
        jmp     _win
:       jsr     restore_level_data
        jmp     load_level
.endproc

.proc reset_level
        jsr     _load_bg
        jsr     restore_level_data
        jsr     setup_level_data
x_coord_reset_handler:
        jsr     _mouse_reset_ref_x
        jmp     _keyboard_reset_ref_x
.endproc

.proc load_level
        jsr     _load_bg
        ; Draw plane once to backup background
        jsr     setup_level_data
        jmp     reset_level::x_coord_reset_handler
.endproc

; Not .proc'ed to jump back to level_logic_done
.proc _main
        lda     #1
        jsr     _init_hgr

        jsr     _init_hgr_base_addrs

        jsr     _init_simple_hgr_addrs

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
        sta     reset_level::x_coord_reset_handler+1
        lda     #>_keyboard_reset_ref_x
        sta     reset_level::x_coord_reset_handler+2

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

        jsr     _clear_hgr_after_input

        jsr     load_level

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
        jmp     game_loop

:       ; Check if we're done with the level
        jsr     _check_level_change

        jsr     _current_level_logic

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

; The only purpose of this is to jsr here so we can jump indirect,
; and still return correctly to caller from the callback.
.proc _current_level_logic
        jmp     (cur_level_logic)
.endproc

.proc die
        jsr     _animate_plane_crash
        jsr     _play_crash
        dec     num_lives
        bne     :+
game_over:
        jsr     restore_level_data
        jmp     reset_game

:       jmp     reset_level
.endproc

; Copy the hgr_baseaddr array of addresses
; to two arrays of low bytes/high bytes for simplicity
.proc _init_simple_hgr_addrs
        ldy     #0
        ldx     #0
:       lda     _hgr_baseaddr,x
        sta     _hgr_low,y
        inx
        lda     _hgr_baseaddr,x
        sta     _hgr_hi,y
        iny
        inx
        bne     :-

:       lda     _hgr_baseaddr+256,x
        sta     _hgr_low,y
        inx
        lda     _hgr_baseaddr+256,x
        sta     _hgr_hi,y
        inx
        iny
        cpy     #192
        bne     :-

        ; Rewrite the mod7_table to have actual modulo-7s
        ; instead of a bit set, because we want to use it
        ; to select a sprite from [0-6] instead of ORing
        ; a bit in an HGR byte (cf ../lib/hgr_addrs.c)
        ldx     #0
        lda     #0
next_mod:
        cmp     #7
        bne     :+
        lda     #0
:       sta     _mod7_table,x
        clc
        adc     #1
        inx
        bne     next_mod
        rts
.endproc

.proc backup_sprite
        lda     cur_sprite
        asl
        tay
        lda     (level_data),y
        sta     ptr2
        iny
        lda     (level_data),y
        sta     ptr2+1

        ldy     #.sizeof(SPRITE_DATA)
        dey

:       lda     (ptr2),y
        sta     level_backup,x
        inx
        dey
        bpl     :-
        rts
.endproc

.proc backup_level_data
        ldx     plane_sprite_num
        stx     cur_sprite

        ldx     #0
:       jsr     backup_sprite
        dec     cur_sprite
        bpl     :-

        rts
.endproc

.proc restore_sprite
        lda     cur_sprite
        asl
        tay
        lda     (level_data),y
        sta     ptr2
        iny
        lda     (level_data),y
        sta     ptr2+1

        ldy     #.sizeof(SPRITE_DATA)
        dey

:       lda     level_backup,x
        sta     (ptr2),y
        inx
        dey
        bpl     :-
        rts
.endproc

.proc restore_level_data
        ldx     plane_sprite_num
        stx     cur_sprite

        ldx     #0
:       jsr     restore_sprite
        dec     cur_sprite
        bpl     :-

        rts
.endproc

.proc setup_level_data
        lda     cur_level
        asl
        tax

        ; Logic
        lda     levels_logic,x
        sta     cur_level_logic
        lda     levels_logic+1,x
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
        lda     sprite_data,x
        sta     level_data
        lda     sprite_data+1,x
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

:       jsr     backup_level_data
        ; Draw each sprite once

        ; Deactivate interrupts for first draw
        php
        sei

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

_hgr_low:        .res 192
_hgr_hi:         .res 192
frame_counter:   .res 1
time_counter:    .res 2
cur_sprite:      .res 1
