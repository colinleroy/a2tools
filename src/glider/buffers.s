        .export  level_backup

        .export  plane_bgbackup
        .export  rubber_band_bgbackup

        .export  sprites_bgbackup

        .include "sprite.inc"
        .include  "plane.gen.inc"
        .include  "rubber_band.gen.inc"

.bss

level_backup:         .res    MAX_SPRITES*.sizeof(SPRITE_DATA)

plane_bgbackup:       .res    plane_BYTES+10
rubber_band_bgbackup: .res    rubber_band_BYTES+10

sprites_bgbackup:     .res    MAX_SPRITES*128
