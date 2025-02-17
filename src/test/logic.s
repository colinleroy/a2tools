        .export     _check_bottom, _check_y_direction

        .import     mouse_x, mouse_y
        .import     levels_data, cur_level
        
        .importzp   _zp6

        .include    "plane.inc"

data_ptr = _zp6

_check_bottom:
        lda       mouse_y
        cmp       #plane_MAX_Y
        bcc       :+
        lda       #plane_MIN_Y
        sta       mouse_y
:       rts

; Return with increment to use (1 or 255 aka -1)
_check_y_direction:
        ; Get current level data to data_ptr
        lda       cur_level
        asl
        tay
        lda       levels_data,y
        sta       data_ptr
        lda       levels_data+1,y
        sta       data_ptr+1

        ; Get number of vents in the level, to X
        ldy       #0
        lda       (data_ptr),y
        tax

        beq       down    ; No vents in the level

do_check:
        ; Check mouse_x against first vent X coords
        iny
        lda       mouse_x
        lda       (data_ptr),y ; lower bound
        cmp       mouse_x
        bcs       next_vent       ; if lb > x, check next
        iny
        lda       (data_ptr),y ; higher bound
        cmp       mouse_x
        bcc       next_vent       ; if hb < x, check next

        lda       #255
        rts
next_vent:
        dex                       ; Check next vent
        bne       do_check        ; if < 0, return down
down:
        lda       #1
        rts
