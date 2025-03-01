        .export     _check_blockers, _check_vents
        .export     _check_plane_bounds, _check_rubber_band_bounds
        .export     _deactivate_sprite, _deactivate_current_sprite
        .export     _fire_rubber_band, _fire_balloon, _fire_knife
        .export     _rubber_band_travel, _balloon_travel, _knife_travel
        .export     _grab_rubber_bands, _grab_battery, _inc_score
        .export     _clock_inc_score
        .import     vents_data, blockers_data, plane_data
        .import     rubber_band_data
        .import     cur_level, frame_counter
        .import     _load_sprite_pointer, _setup_sprite_pointer, _clear_and_draw_sprite
        .import     num_rubber_bands, num_battery, cur_score
        .import     _play_croutch

        .importzp   tmp1, tmp2, tmp3, ptr4

        .include    "balloon.gen.inc"
        .include    "knife.gen.inc"
        .include    "plane.gen.inc"
        .include    "rubber_band.gen.inc"
        .include    "sprite.inc"
        .include    "plane_coords.inc"
        .include    "level_data_ptr.inc"

; Return with carry set if mouse coords in box
; (data_ptr),y to y+3 contains box coords (start X, width, start Y, height)
; Always return with Y at end of coords so caller knows where Y is at.
; Trashes A, updates Y, does not touch X
_check_plane_bounds:
        ; plane_x,plane_y is the top-left corner of the plane
        ; compute bottom-right corner
        clc
        lda     plane_x
        sta     check_bounds_sx+1
        adc     #plane_WIDTH
        sta     check_bounds_ex+1
        lda     plane_y
        sta     check_bounds_sy+1
        adc     #plane_HEIGHT
        sta     check_bounds_ey+1

check_bounds:
        ; Check right of plane against first blocker X coords
        lda     (data_ptr),y
        iny                       ; Inc Y now so we know how much to skip
check_bounds_ex:
        cmp     #$FF              ; lower X bound
        bcs     out_skip_y        ; if lb > x, we're out of box

        ; Check left of plane against right of box
        adc     (data_ptr),y      ; higher X bound (lower+width)
check_bounds_sx:
        cmp     #$FF              ; Patched with X coordinate
        bcc     out_skip_y        ; if hb < x, we're out of box

        ; Check bottom of plane against first blocker Y coords
        iny
        lda     (data_ptr),y      ; lower Y bound
        iny                       ; Inc Y now so we have nothing to skip
check_bounds_ey:
        cmp     #$FF
        bcs     out               ; if lb > y, we're out of box

        ; Check top of plane against lower bound of blocker
        adc     (data_ptr),y      ; higher Y bound (lower+height)
check_bounds_sy:
        cmp     #$FF
        bcc     out               ; if hb < y, we're out of box

        rts                       ; We're in the box (return, carry already set)

_check_rubber_band_bounds:
        clc
        lda     rubber_band_data+SPRITE_DATA::X_COORD
        sta     check_bounds_sx+1
        adc     #rubber_band_WIDTH
        sta     check_bounds_ex+1
        lda     rubber_band_data+SPRITE_DATA::Y_COORD
        sta     check_bounds_sy+1
        adc     #rubber_band_HEIGHT
        sta     check_bounds_ey+1
        jmp     check_bounds

out_skip_y:
        iny
        iny
out:
        clc
        rts

; Return with carry set if in an obstacle
_check_blockers:
        ; Get current level data to data_ptr
        lda     cur_level
        asl
        tay
        lda     blockers_data,y
        sta     data_ptr
        lda     blockers_data+1,y
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
        rts                       ; Carry already set

next_blocker:
        dex                       ; Check next obstacle?
        bne     do_check_blocker

no_blockers:
        clc
        rts

; Return with Y increment to use
_check_vents:
        ; Get current level data to data_ptr
        lda     cur_level
        asl
        tay
        lda     vents_data,y
        sta     data_ptr
        lda     vents_data+1,y
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

_deactivate_rubber_band:
        lda     #0

_deactivate_sprite:
        jsr     _load_sprite_pointer
        jsr     _setup_sprite_pointer
_deactivate_current_sprite:
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
        jmp     _clear_and_draw_sprite

_fire_rubber_band:
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

_rubber_band_travel:
        lda     rubber_band_data+SPRITE_DATA::ACTIVE
        beq     no_travel

        lda     rubber_band_data+SPRITE_DATA::X_COORD
        cmp     #(250-plane_WIDTH-1)
        bcs     _deactivate_rubber_band

        adc     #3
        sta     rubber_band_data+SPRITE_DATA::X_COORD
no_travel:
        rts

_fire_balloon:
        cpx     frame_counter
        bne     :+

        jsr     _load_sprite_pointer
        ldy     #SPRITE_DATA::ACTIVE
        lda     (cur_sprite_ptr),y
        bne     :+

        lda     #1
        sta     (cur_sprite_ptr),y
        lda     #(191-balloon_HEIGHT)
        ldy     #SPRITE_DATA::Y_COORD
        sta     (cur_sprite_ptr),y

:       rts

_balloon_travel:
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
        jmp     _deactivate_sprite
:       rts


_fire_knife:
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

_knife_travel:
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
        jsr     _deactivate_sprite

        ; Restore its original coords
        ldy     #SPRITE_DATA::STATE_BACKUP
        lda     (cur_sprite_ptr),y
        ldy     #SPRITE_DATA::X_COORD
        sta     (cur_sprite_ptr),y

        ldy     #SPRITE_DATA::STATE_BACKUP+1
        lda     (cur_sprite_ptr),y
        ldy     #SPRITE_DATA::Y_COORD
        sta     (cur_sprite_ptr),y
        rts

knife_left:
        ; Now left it
        ldy     #SPRITE_DATA::X_COORD
        lda     (cur_sprite_ptr),y
        sec
        sbc     #1
        sta     (cur_sprite_ptr),y
        beq     knife_out
        rts

_grab_rubber_bands:
        clc
        adc     num_rubber_bands
        bcs     :+
        sta     num_rubber_bands
        rts
:       lda     #$FF
        sta     num_rubber_bands
        rts

_clock_inc_score:
        jsr     _play_croutch
        lda     #5
_inc_score:
        clc
        adc     cur_score
        sta     cur_score
        bcc     :+
        inc     cur_score+1
:       rts

_grab_battery:
        clc
        adc     num_battery
        bcs     :+
        sta     num_battery
        rts
:       lda     #$FF
        sta     num_battery
        rts
