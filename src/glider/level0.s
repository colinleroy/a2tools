        .export   level0_sprites, level0_blockers, level0_vents

        .import   clock0_backup
        .import   clock1_backup
        .import   clock2_backup

        .import   _clock, _clock_mask

        .export  level0_clock1_data

        .import   plane_data
        .include  "clock.inc"
        .include  "plane.inc"

.data

; Do not place anything after X= 224 to avoid overflow
; in the hitbox
level0_clock0_data:
                  .byte 1             ; active
                  .byte 0             ; deadly
                  .byte 110           ; x
                  .byte clock_WIDTH
                  .byte 107           ; y
                  .byte clock_HEIGHT
                  .byte 110           ; prev_x
                  .byte 107           ; prev_y
                  .byte clock_BYTES-1 ; bytes of sprite - 1
                  .byte clock_WIDTH/7 ; width of sprite in bytes
                  .addr clock0_backup ; background buffer
                  .addr _clock        ; clock sprites
                  .addr _clock_mask   ; clock masks

level0_clock1_data:
                  .byte 1             ; active
                  .byte 1             ; deadly
                  .byte 140           ; x
                  .byte clock_WIDTH
                  .byte 107           ; y
                  .byte clock_HEIGHT
                  .byte 140           ; prev_x
                  .byte 107           ; prev_y
                  .byte clock_BYTES-1 ; bytes of sprite - 1
                  .byte clock_WIDTH/7 ; width of sprite in bytes
                  .addr clock1_backup ; background buffer
                  .addr _clock        ; clock sprites
                  .addr _clock_mask   ; clock masks

level0_clock2_data:
                  .byte 1             ; active
                  .byte 0             ; deadly
                  .byte 200           ; x
                  .byte clock_WIDTH
                  .byte 177           ; y
                  .byte clock_HEIGHT
                  .byte 200           ; prev_x
                  .byte 177           ; prev_y
                  .byte clock_BYTES-1 ; bytes of sprite - 1
                  .byte clock_WIDTH/7 ; width of sprite in bytes
                  .addr clock2_backup ; background buffer
                  .addr _clock        ; clock sprites
                  .addr _clock_mask   ; clock masks

.rodata

level0_sprites:   .byte   4
level0_sprites_data:
                  .addr   level0_clock0_data
                  .addr   level0_clock1_data
                  .addr   level0_clock2_data
                  .addr   plane_data

level0_vents:     .byte   2
level0_vents_data:
                  ; Five bytes per vent (start X, width, start Y, height, direction)
                  ; Direction = What to add to mouse_y
                  ; Watch out - start Y must be >= plane_HEIGHT
                  .byte   40,  20,  plane_HEIGHT+1,   191-plane_HEIGHT, $FF ; Up all the way
                  .byte   227, 20,  plane_HEIGHT+1,   191-plane_HEIGHT, $FF ; Up all the way

level0_blockers:  .byte   4
level0_blockers_data:
                  ; Four bytes per blocker (start X, width, start Y, height)
                  .byte   102, 23,  91,  31    ; Mac
                  .byte   102, 92,  121, 6     ; Desk
                  .byte   147, 3,   126, 64    ; Foot
                  .byte   0,   255, 191, 1     ; Floor
