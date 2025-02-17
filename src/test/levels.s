                  .export vents_data, blockers_data
                  .export num_levels, cur_level

.data

cur_level:        .byte   0
.rodata

level0_vents:     .byte   2
level0_vents_x:   .byte   20,  58
                  .byte   215, 250

level0_blockers:  .byte   4
level0_blockers_coords:
                  .byte   67, 126     ; Mac X
                  .byte   77, 127     ; Mac Y
                  .byte   67, 194     ; Desk X
                  .byte   102, 127    ; Desk Y
                  .byte   112, 150    ; Foot X
                  .byte   127, 192    ; Foot Y
                  .byte   0, 255      ; Floor X
                  .byte   171, 192    ; Floor Y

num_levels:       .byte   1

vents_data:
                  .addr level0_vents

blockers_data:
                  .addr level0_blockers
