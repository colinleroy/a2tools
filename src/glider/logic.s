        .export     _check_blockers, _check_vents
        .export     _check_mouse_bounds
        .export     _deactivate_sprite

        .import     vents_data, blockers_data, plane_data
        .import     cur_level
        .import     _setup_sprite_pointer, _clear_and_draw_sprite
        
        .importzp   _zp6, tmp1, tmp2, ptr4

        .include    "plane.inc"
        .include    "sprite.inc"
        .include    "plane_coords.inc"
        .include    "level_data_ptr.inc"

; Return with carry set if mouse coords in box
; (data_ptr),y to y+3 contains box coords (start X, width, start Y, height)
; Always return with Y at end of coords so caller knows where Y is at.
; Trashes A, updates Y, does not touch X
_check_mouse_bounds:
        ; plane_x,plane_y is the top-left corner of the plane
        ; compute bottom-right corner
        clc
        lda     plane_x
        adc     #plane_WIDTH
        sta     tmp1
        lda     plane_y
        adc     #plane_HEIGHT
        sta     tmp2

        ; Check right of plane against first blocker X coords
        lda     (data_ptr),y
        iny                       ; Inc Y now so we know how much to skip
        cmp     tmp1              ; lower X bound
        bcs     out_skip_y        ; if lb > x, we're out of box

        ; Check left of plane against right of box
        adc     (data_ptr),y      ; higher X bound (lower+width)
        cmp     plane_x
        bcc     out_skip_y        ; if hb < x, we're out of box

        ; Check bottom of plane against first blocker Y coords
        iny
        lda     (data_ptr),y      ; lower Y bound
        iny                       ; Inc Y now so we have nothing to skip
        cmp     tmp2
        bcs     out               ; if lb > y, we're out of box

        ; Check top of plane against lower bound of blocker
        adc     (data_ptr),y      ; higher Y bound (lower+height)
        cmp     plane_y
        bcc     out               ; if hb < y, we're out of box

        rts                       ; We're in the box (return, carry already set)

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
        jsr     _check_mouse_bounds
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
        jsr     _check_mouse_bounds
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

_deactivate_sprite:
        jsr     _setup_sprite_pointer
        ldy     #SPRITE_DATA::ACTIVE
        lda     #0
        sta     (cur_sprite_ptr),y
        jmp     _clear_and_draw_sprite
