        .export   _main
        .export   _hgr_low, _hgr_hi
        .export   frame_counter, time_counter
        .export   level_logic_done

        .import   _exit
        .import   _init_hgr, _init_mouse
        .import   _init_hgr_base_addrs, _hgr_baseaddr
        .import   _bzero
        .import   pushax
        .import   _mod7_table
        .import   cur_level, num_levels
        .import   _draw_sprite, _clear_and_draw_sprite
        .import   _load_sprite_pointer
        .import   _setup_sprite_pointer
        .import   _check_blockers, _check_vents
        .import   _check_plane_bounds
        .import   _check_rubber_band_bounds
        .import   _load_bg
        .import   _deactivate_sprite
        .import   _inc_score
        .import   _animate_plane_crash

        .import   level_backup
        .import   levels_logic, cur_level_logic

        .import   _fire_rubber_band
        .import   _rubber_band_travel
        .import   num_lives, num_rubber_bands, num_battery, cur_score
        .import   plane_sprite_num

        .import   reset_mouse, mouse_reset_ref_x, hz
        .import   mouse_wait_vbl, mouse_calibrate_hz, mouse_update_ref_x
        .import   mouse_check_fire
        .import   sprite_data, plane_data, rubber_band_data

        .import   _print_dashboard, _print_level_end, _clear_hgr_screen

        .import   _play_bubble, _play_crash

        .importzp ptr2, ptr4

        .include  "apple2.inc"
        .include  "plane.gen.inc"
        .include  "sprite.inc"
        .include  "level_data_ptr.inc"
        .include  "plane_coords.inc"
        .include  "constants.inc"

_main:

        jsr     _clear_hgr_screen

        lda     #1
        jsr     _init_hgr

        jsr     _init_hgr_base_addrs

        jsr     _init_simple_hgr_addrs

        jsr     _init_mouse
        bcc     :+                ; Calibrate HZ with mouse if possible
        jmp     :++               ; Otherwise we'll do with VBL

:       ; Setup mouse input
        jsr     mouse_calibrate_hz


:       jsr     load_level

game_loop:
        ; the WAI of the poor
        ; because I don't understand how WAI works
        ; and I want to keep things 6502-ok
wait_vbl_handler:
        jsr     mouse_wait_vbl
;
; DRAW SPRITES FIRST
;
        lda     plane_sprite_num
        sta     cur_sprite

        ; Always draw the plane
        jsr     _load_sprite_pointer
        jsr     _setup_sprite_pointer
        jsr     _clear_and_draw_sprite

        lda     frame_counter     ; Draw only half sprites
        and     #01
        beq     :+
        dec     cur_sprite
:

draw_next_sprite:
        dec     cur_sprite
        lda     cur_sprite
        bmi     draw_dashboard    ; All done!

        jsr     _load_sprite_pointer
        bne     dec_sprite_draw   ; Only draw dynamic sprites

        ldy     #SPRITE_DATA::ACTIVE
        lda     (cur_sprite_ptr),y
        beq     dec_sprite_draw   ; Only draw active sprites

        jsr     _setup_sprite_pointer
        jsr     _clear_and_draw_sprite

dec_sprite_draw:
        dec     cur_sprite        ; Skip a sprite
        bpl     draw_next_sprite

draw_dashboard:
        lda     frame_counter     ; Draw dashboard on odd frames
        and     #01
        beq     game_logic
        jsr     _print_dashboard

;
; GENERAL GAME LOGIC
;
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
x_coord_handler:
        jsr     mouse_update_ref_x
        sta     plane_x

        jsr     _check_vents            ; Returns with offset to add to plane_y
        clc
        adc     plane_y
        cmp     #plane_MAX_Y
        bcc     :+

        ; We're on the floor
        lda     #plane_MAX_Y

:       sta     plane_y

        ; Check obstacles
        jsr     _check_blockers
        bcc     move_checks_done

        ; We got in an obstacle
die:
        jsr     _animate_plane_crash
        jsr     _play_crash
        dec     num_lives
        bne     :+
game_over:
        jsr     restore_level_data
        jsr     reset_game
        jmp     game_loop

:       jsr     reset_level

move_checks_done:
        ; Check level change now
        ; First by cheat-code
        lda     KBD
        bpl     :+
        bit     KBDSTRB
        bit     BUTN0
        bpl     :+

        ; Open-Apple is down. Clear high bit, substract 'a' and go to level
        and     #$7F
        sec
        sbc     #'a'
        jsr     go_to_level
        jmp     level_logic

        ; Then by plane X coord
:       lda     plane_x
        bne     :+
        jsr     prev_level
        jmp     level_logic

:       .assert (280-plane_WIDTH) .mod $2 = $0, error
        cmp     #(280-plane_WIDTH)
        bcc     level_logic
        ; We finished the level!
        jsr     next_level

level_logic:
        jmp     (cur_level_logic)

; The jump target back from level logic handler
level_logic_done:
        ; Check if we should fire a rubber band
fire_handler:
        jsr     mouse_check_fire
        bcc     :+
        jsr     _fire_rubber_band

:       jsr     _rubber_band_travel

;
; COLLISION CHECKS
;
collision_checks:
        inc     frame_counter

        ldx     plane_sprite_num
        stx     cur_sprite

check_next_sprite:
        dec     cur_sprite
        lda     cur_sprite
        bpl     :+                ; Are we done?
        jmp     game_loop

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
        bcc     check_next_sprite

        ; We're in the sprite box, is it active?
        ldy     #SPRITE_DATA::ACTIVE
        lda     (cur_sprite_ptr),y
        beq     check_next_sprite  ; No, we're good

        ; Is it deadly?
        ldy     #SPRITE_DATA::DEADLY
        lda     (cur_sprite_ptr),y
        beq     destroy_sprite    ; No, grab it (but don't get score for it)
        jmp     die               ; Yes, die

destroy_sprite_with_rubber_band:
        lda     #0                ; Deactivate rubber band
        jsr     _deactivate_sprite
        jsr     _play_bubble      ; Play sound
        lda     #DESTROY_BONUS
        jsr     _inc_score

        ; Deactivate it
destroy_sprite:
        lda     cur_sprite
        jsr     _deactivate_sprite

        jmp     check_next_sprite

        ; Unreachable code.
        brk

; Copy the hgr_baseaddr array of addresses
; to two arrays of low bytes/high bytes for simplicity
_init_simple_hgr_addrs:
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

backup_sprite:
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

backup_level_data:
        ldx     plane_sprite_num
        stx     cur_sprite

        ldx     #0
:       jsr     backup_sprite
        dec     cur_sprite
        bpl     :-

        rts

restore_sprite:
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

restore_level_data:
        ldx     plane_sprite_num
        stx     cur_sprite

        ldx     #0
:       jsr     restore_sprite
        dec     cur_sprite
        bpl     :-

        rts

setup_level_data:
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

:
        jsr     backup_level_data
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

reset_game:
        lda     #3
        sta     num_lives
        lda     #0
        sta     cur_level
        sta     num_rubber_bands
        sta     num_battery
        sta     cur_score
        sta     cur_score+1
        jmp     load_level

prev_level:
        rts ; UNSURE IF I WANT TO GO BACK IN LEVELS?
        ; We restore level data, in case we die later
        ; and come back to this level.
        lda     cur_level
        bne     :+
        rts
:       jsr     restore_level_data
        dec     cur_level
        jmp     load_level

next_level:
        inc     cur_level
        ; Print the time bonus
        jsr     _print_level_end
        ; We restore level data, in case we die later
        ; and come back to this level.
        lda     cur_level
go_to_level:
        sta     cur_level
        cmp     num_levels
        bcc     :+
        jmp     _win
:       jsr     restore_level_data
        jmp     load_level

reset_level:
        jsr     _load_bg
        jsr     restore_level_data
        jsr     setup_level_data
x_coord_reset_handler:
        jmp     mouse_reset_ref_x

load_level:
        jsr     _load_bg
        ; Draw plane once to backup background
        jsr     setup_level_data
        jmp     x_coord_reset_handler

_win:
        jmp     _exit

        .bss

_hgr_low:        .res 192
_hgr_hi:         .res 192
frame_counter:   .res 1
time_counter:    .res 2
cur_sprite:      .res 1
