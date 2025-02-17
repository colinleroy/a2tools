        .export     _check_bottom

        .import     mouse_y
        .include    "plane.inc"

_check_bottom:
        lda       mouse_y
        cmp       #plane_MAX_Y
        bcc       :+
        lda       #plane_MIN_Y
        sta       mouse_y
:       rts
