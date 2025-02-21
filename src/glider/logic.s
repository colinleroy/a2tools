        .export     _check_blockers, _check_vents

        .import     vents_data, blockers_data, plane_data
        .import     cur_level
        
        .importzp   _zp6

        .include    "plane.inc"
        .include    "sprite.inc"
        .include    "plane_coords.inc"

data_ptr = _zp6

; Return with carry set if mouse coords in box
; (data_ptr),y to y+3 contains box coords (start X, end X, start Y, end Y)
; Always return with Y at end of coords so caller knows where Y is at.
; Trashes A, updates Y, does not touch X
check_mouse_bounds:
        ; Check plane_x against first blocker X coords
        lda       (data_ptr),y
        cmp       plane_x         ; lower X bound
        iny                       ; Inc Y now so we know how much to skip
        bcs       out_skip_y      ; if lb > x, we're out of box

        adc       (data_ptr),y    ; higher X bound (lower+width)
        cmp       plane_x
        bcc       out_skip_y      ; if hb < x, we're out of box

        ; Check plane_y against first blocker Y coords
        iny
        lda       (data_ptr),y    ; lower Y bound
        iny                       ; Inc Y now so we have nothing to skip
        cmp       plane_y
        bcs       out             ; if lb > y, we're out of box

        adc       (data_ptr),y    ; higher Y bound (lower+height)
        cmp       plane_y
        bcc       out             ; if hb < y, we're out of box

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
        lda       cur_level
        asl
        tay
        lda       blockers_data,y
        sta       data_ptr
        lda       blockers_data+1,y
        sta       data_ptr+1

        ; Get number of blockers in the level, to X
        ldy       #0
        lda       (data_ptr),y
        tax

        beq       no_blockers

do_check_blocker:
        iny
        jsr       check_mouse_bounds
        bcc       next_blocker

        ; We're in an obstacle
        lda       $C030
        rts                       ; Carry already set

next_blocker:
        dex                       ; Check next obstacle?
        bne       do_check_blocker

no_blockers:
        clc
        rts

; Return with Y increment to use
_check_vents:
        ; Get current level data to data_ptr
        lda       cur_level
        asl
        tay
        lda       vents_data,y
        sta       data_ptr
        lda       vents_data+1,y
        sta       data_ptr+1

        ; Get number of vents in the level, to X
        ldy       #0
        lda       (data_ptr),y
        tax

        beq       go_down

do_check_vent:
        iny
        jsr       check_mouse_bounds
        bcc       next_vent

        ; We're in a vent tunnel, load its Y delta
        iny
        lda       (data_ptr),y
        rts

next_vent:
        iny
        dex                       ; Check next vent?
        bne       do_check_vent

go_down:
        lda       #1              ; Go down normal
        rts
