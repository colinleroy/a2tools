                  .export vents_data, blockers_data
                  .export num_levels, cur_level
                  .export levels_logic, cur_level_logic

                  .export sprite_data, plane_data
                  .import _plane, _plane_mask, plane_backup

                  .import  level0_sprites, level0_blockers
                  .import  level0_vents, level0_logic
                  .import  level1_sprites, level1_blockers
                  .import  level1_vents, level1_logic

                  .include "clock.inc"
                  .include "plane.inc"

.data

; There is always a single plane
plane_data:
                  .byte 1             ; active
                  .byte 0             ; deadly
                  .byte plane_MIN_X   ; x
                  .byte plane_WIDTH
                  .byte plane_MIN_Y   ; y
                  .byte plane_HEIGHT
                  .byte plane_MAX_X   ; prev_x
                  .byte plane_MAX_Y   ; prev_y
                  .byte plane_BYTES-1 ; bytes of sprite - 1
                  .byte plane_WIDTH/7 ; width of sprite in bytes
                  .addr plane_backup  ; background buffer
                  .addr _plane        ; plane sprites
                  .addr _plane_mask   ; plane masks

cur_level:        .byte   0
cur_level_logic:  .addr   $FFFF

.rodata

num_levels:       .byte   2

sprite_data:
                  .addr level0_sprites
                  .addr level1_sprites

vents_data:
                  .addr level0_vents
                  .addr level1_vents

blockers_data:
                  .addr level0_blockers
                  .addr level1_blockers

levels_logic:
                  .addr level0_logic
                  .addr level1_logic
