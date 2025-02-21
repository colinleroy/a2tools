                  .export vents_data, blockers_data
                  .export num_levels, cur_level

                  .export sprite_data, plane_data
                  .import _plane, _plane_mask, plane_backup

                  .import  level0_sprites, level0_blockers, level0_vents

                  .include "clock.inc"
                  .include "plane.inc"

.data

; There is always a single plane
plane_data:
                  .byte 0             ; x
                  .byte 0             ; y
                  .byte plane_WIDTH
                  .byte plane_HEIGHT
                  .byte 0             ; prev_x
                  .byte 0             ; prev_y
                  .byte plane_BYTES-1 ; bytes of sprite - 1
                  .byte plane_WIDTH/7 ; width of sprite in bytes
                  .addr plane_backup  ; background buffer
                  .addr _plane        ; plane sprites
                  .addr _plane_mask   ; plane masks

cur_level:        .byte   0

.rodata

num_levels:       .byte   1

sprite_data:
                  .addr level0_sprites

vents_data:
                  .addr level0_vents

blockers_data:
                  .addr level0_blockers
