                  .export levels_data, num_levels, cur_level

.data

cur_level:        .byte   0
.rodata

level0_vents:     .byte   2
level0_vents_x:   .byte   20,  58
                  .byte   215, 250


num_levels:       .byte   1

levels_data:
                  .addr level0_vents
