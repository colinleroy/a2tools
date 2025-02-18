                  .export vents_data, blockers_data
                  .export num_levels, cur_level

                  .include "plane.inc"
.data

cur_level:        .byte   0
.rodata

level0_vents:     .byte   2
level0_vents_x:   .byte   41-plane_WIDTH,  61
                  .byte   227-plane_WIDTH, 247

level0_blockers:  .byte   4
level0_blockers_coords:
                  .byte   102-plane_WIDTH,  126    ; Mac X
                  .byte   91-plane_HEIGHT,  127    ; Mac Y
                  .byte   102-plane_WIDTH,  194    ; Desk X
                  .byte   121-plane_HEIGHT, 127    ; Desk Y
                  .byte   147-plane_WIDTH,  150    ; Foot X
                  .byte   126-plane_HEIGHT, 192    ; Foot Y
                  .byte   0, 255                   ; Floor X
                  .byte   191-plane_HEIGHT, 191    ; Floor Y

num_levels:       .byte   1

vents_data:
                  .addr level0_vents

blockers_data:
                  .addr level0_blockers
