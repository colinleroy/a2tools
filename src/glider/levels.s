                  .export vents_data, blockers_data
                  .export num_levels, cur_level
                  .export levels_logic, cur_level_logic

                  .export num_lives, num_rubber_bands, num_battery
                  .export cur_score, plane_sprite_num
                  .export sprite_data, plane_data, rubber_band_data

                  .import _plane, _plane_mask
                  .import _rubber_band, _rubber_band_mask

                  .import  level0_sprites, level0_blockers
                  .import  level0_vents, level0_logic
                  .import  level1_sprites, level1_blockers
                  .import  level1_vents, level1_logic
                  .import  level2_sprites, level2_blockers
                  .import  level2_vents, level2_logic
                  .import  level3_sprites, level3_blockers
                  .import  level3_vents, level3_logic
                  .import  level4_sprites, level4_blockers
                  .import  level4_vents, level4_logic

                  .import  plane_bgbackup
                  .import  rubber_band_bgbackup

                  .include "clock.gen.inc"
                  .include "plane.gen.inc"
                  .include "rubber_band.gen.inc"

.data

; There is always a single plane
plane_data:
                  .byte 1             ; active
                  .byte 0             ; deadly
                  .byte 1             ; destroyable
                  .byte 0             ; static
                  .byte plane_MIN_X   ; x
                  .byte plane_WIDTH
                  .byte plane_MIN_Y   ; y
                  .byte plane_HEIGHT
                  .byte plane_MAX_X   ; prev_x
                  .byte plane_MAX_Y   ; prev_y
                  .byte plane_BYTES-1 ; bytes of sprite - 1
                  .byte plane_BPLINE-1; width of sprite in bytes
                  .addr _plane        ; plane sprites
                  .addr _plane_mask   ; plane masks
                  .byte 0             ; deactivate data
                  .addr $0000         ; deactivate cb
                  .word $0000         ; state backup
                  .addr plane_bgbackup
                  .byte 0               ; need clear

; There is always a single rubber band
rubber_band_data:
                  .byte 0                   ; active
                  .byte 1                   ; deadly
                  .byte 0                   ; destroyable
                  .byte 0                   ; static
                  .byte rubber_band_MIN_X   ; x
                  .byte rubber_band_WIDTH
                  .byte rubber_band_MIN_Y   ; y
                  .byte rubber_band_HEIGHT
                  .byte rubber_band_MAX_X   ; prev_x
                  .byte rubber_band_MAX_Y   ; prev_y
                  .byte rubber_band_BYTES-1 ; bytes of sprite - 1
                  .byte rubber_band_BPLINE-1; width of sprite in bytes
                  .addr _rubber_band        ; band sprites
                  .addr _rubber_band_mask   ; band masks
                  .byte 0
                  .addr $0000
                  .word $0000
                  .addr rubber_band_bgbackup
                  .byte 0               ; need clear

cur_level:        .byte   0
num_lives:        .byte   3
cur_score:        .word   0
num_rubber_bands: .byte   0
num_battery:      .byte   0

cur_level_logic:  .addr   $FFFF
plane_sprite_num: .byte   0

.rodata

num_levels:       .byte   5

sprite_data:
                  .addr level0_sprites
                  .addr level1_sprites
                  .addr level2_sprites
                  .addr level3_sprites
                  .addr level4_sprites

vents_data:
                  .addr level0_vents
                  .addr level1_vents
                  .addr level2_vents
                  .addr level3_vents
                  .addr level4_vents

blockers_data:
                  .addr level0_blockers
                  .addr level1_blockers
                  .addr level2_blockers
                  .addr level3_blockers
                  .addr level4_blockers

levels_logic:
                  .addr level0_logic
                  .addr level1_logic
                  .addr level2_logic
                  .addr level3_logic
                  .addr level4_logic
