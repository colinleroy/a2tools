        .export     _check_bounds, _check_y_direction

        .import     mouse_x, mouse_y
        .import     vents_data, blockers_data, cur_level
        
        .importzp   _zp6

        .include    "plane.inc"

data_ptr = _zp6

; Return with carry set if in an obstacle

_check_bounds:
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
        ; Check mouse_x against first blocker X coords
        iny
        lda       (data_ptr),y    ; lower X bound
        iny                       ; Inc Y now so we know how much to skip
        cmp       mouse_x
        bcs       next_blocker_skip_y    ; if lb > x, check next

        lda       (data_ptr),y    ; higher X bound
        cmp       mouse_x
        bcc       next_blocker_skip_y    ; if hb < x, check next

        ; Check mouse_y against first blocker Y coords
        iny
        lda       (data_ptr),y    ; lower Y bound
        iny                       ; Inc Y now so we know how much to skip
        cmp       mouse_y
        bcs       next_blocker    ; if lb > y, check next

        lda       (data_ptr),y    ; higher Y bound
        cmp       mouse_y
        bcc       next_blocker    ; if hb < y, check next

        ; We're in an obstacle
        lda       $C030
        sec
        rts
next_blocker:
        dex                       ; Check next obstacle
        bne       do_check_blocker; if < 0, return down
        clc
        rts

next_blocker_skip_y:
        iny
        iny
        dex
        bne       do_check_blocker; if < 0, return down

no_blockers:
        clc
        rts

; Return with increment to use (1 or 255 aka -1)
_check_y_direction:
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

        beq       down    ; No vents in the level

do_check:
        ; Check mouse_x against first vent X coords
        iny
        lda       (data_ptr),y ; lower bound
        cmp       mouse_x
        iny
        bcs       next_vent       ; if lb > x, check next

        lda       (data_ptr),y ; higher bound
        cmp       mouse_x
        bcc       next_vent       ; if hb < x, check next

        lda       mouse_y
        beq       zero_y_dir
        lda       #255
        rts
zero_y_dir:
        lda       #0              ; Don't go higher than zero
        rts                       ; or lower than max Y

next_vent:
        dex                       ; Check next vent
        bne       do_check        ; if < 0, return down
down:
        lda       mouse_y
        cmp       #plane_MAX_Y
        beq       zero_y_dir
        lda       #1
        rts
