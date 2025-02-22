        .export  plane_backup
        .export  clock0_backup
        .export  level_backup

        .include "plane.inc"
        .include "clock.inc"
        .include "sprite.inc"
.bss

plane_backup:     .res    plane_BYTES
clock0_backup:    .res    clock_BYTES

level_backup:     .res    8*.sizeof(SPRITE_DATA)
