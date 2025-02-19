                  .export vents_data, blockers_data
                  .export num_levels, cur_level

                  .include "plane.inc"
.data

cur_level:        .byte   0
.rodata

level0_vents:     .byte   2
level0_vents_data:
                  ; Five bytes per vent (start X, end X, start Y, end Y, direction)
                  ; Direction = What to add to mouse_y
                  .byte   37-plane_WIDTH,  57,  0,   192, $FF ; Up all the way
                  .byte   227-plane_WIDTH, 247, 0,   192, $FF ; Up all the way

                  ; Examples of vents
                  ;.byte   41-plane_WIDTH,  61,  0,   121-plane_HEIGHT, $FF ; Up from desk height to top
                  ;.byte   102-plane_WIDTH, 126, 0,   192             , $2  ; Fast down all the way
                  ;.byte   200-plane_WIDTH, 220, 128, 192             , $FF ; Up from bottom to 128

level0_blockers:  .byte   4
level0_blockers_data:
                  ; Four bytes per blocker (start X, end X, start Y, end Y)
                  .byte   102-plane_WIDTH, 126, 91-plane_HEIGHT,  127    ; Mac
                  .byte   102-plane_WIDTH, 194, 121-plane_HEIGHT, 127    ; Desk
                  .byte   147-plane_WIDTH, 150, 126-plane_HEIGHT, 192    ; Foot
                  .byte   0,               255, 191-plane_HEIGHT, 191    ; Floor

num_levels:       .byte   1

vents_data:
                  .addr level0_vents

blockers_data:
                  .addr level0_blockers
