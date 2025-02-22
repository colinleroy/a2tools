        .export  plane_backup
        .export  clock0_backup
        .export  clock1_backup
        .export  clock2_backup
        .export  clock3_backup
        .export  clock4_backup
        .export  clock5_backup
        .export  clock6_backup
        .export  level_backup

        .include "plane.inc"
        .include "clock.inc"
        .include "sprite.inc"
.bss

plane_backup:     .res    plane_BYTES
clock0_backup:    .res    clock_BYTES
clock1_backup:    .res    clock_BYTES
clock2_backup:    .res    clock_BYTES
clock3_backup:    .res    clock_BYTES
clock4_backup:    .res    clock_BYTES
clock5_backup:    .res    clock_BYTES
clock6_backup:    .res    clock_BYTES

level_backup:     .res    8*.sizeof(SPRITE_DATA)
