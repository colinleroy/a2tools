        .export  level_backup

        .export  plane_bgbackup
        .export  rubber_band_bgbackup

        .export  balloon0_bgbackup
        .export  battery0_bgbackup
        .export  clock0_bgbackup
        .export  knife0_bgbackup
        .export  knife1_bgbackup
        .export  rubber_box0_bgbackup
        .export  socket0_bgbackup
        .export  switch0_bgbackup

        .include "sprite.inc"
        .include  "battery.gen.inc"
        .include  "balloon.gen.inc"
        .include  "clock.gen.inc"
        .include  "knife.gen.inc"
        .include  "plane.gen.inc"
        .include  "rubber_band.gen.inc"
        .include  "rubber_box.gen.inc"
        .include  "socket.gen.inc"
        .include  "switch.gen.inc"

.bss

level_backup:         .res    MAX_SPRITES*.sizeof(SPRITE_DATA)

plane_bgbackup:       .res    plane_BYTES+10
rubber_band_bgbackup: .res    rubber_band_BYTES+10

balloon0_bgbackup:    .res    balloon_BYTES+10
battery0_bgbackup:    .res    battery_BYTES+10
clock0_bgbackup:      .res    clock_BYTES+10
knife0_bgbackup:      .res    knife_BYTES+10
knife1_bgbackup:      .res    knife_BYTES+10
rubber_box0_bgbackup: .res    rubber_box_BYTES+10
socket0_bgbackup:     .res    socket_BYTES+10
switch0_bgbackup:     .res    switch_BYTES+10
