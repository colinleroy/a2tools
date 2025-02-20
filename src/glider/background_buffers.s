        .export  plane_backup
        .export  clock0_backup
        .export  clock1_backup
        .export  clock2_backup

        .include "plane.inc"
        .include "clock.inc"
.bss

plane_backup:     .res    plane_BYTES
clock0_backup:    .res    clock_BYTES
clock1_backup:    .res    clock_BYTES
clock2_backup:    .res    clock_BYTES
